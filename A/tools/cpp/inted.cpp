#include "mpublic.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <time.h>

using namespace idc;

#define MAX_EVENTS 1024
#define MAXSOCK 1024
#define BUFFER_SIZE 5000

clogfile logfile;
// 路由结构体
struct st_route {
    int inport;
    char ipaddr[31];
    int outport;
    int listensocket;
};

std::vector<struct st_route> vroute;
bool loadroute(char *conf); // 读/etc/inetd.conf文件

int initserver(const int inport); // 初始化服务端
int clientsocks[MAXSOCK] = {0};   // 存放每个socket连接对端的socket的值，初始化全0
int clientatime[MAXSOCK] = {0};   // 存放每个socket连接最后一次收发报文的时间，初始化全0

// 设置文件描述符为非阻塞
int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        logfile.write("fcntl F_GETFL failed for fd %d\n", fd);
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        logfile.write("fcntl F_SETFL failed for fd %d\n", fd);
        return -1;
    }
    return 0;
}

int epoll_fd;

// 向目标地址和端口发起socket连接（修复非阻塞connect逻辑）
int conntodst(const char *ip, const int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        logfile.write("create dst socket failed: %s\n", strerror(errno));
        return -1;
    }

    // 设置非阻塞
    if (set_nonblock(sockfd) < 0) {
        close(sockfd);
        return -1;
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    // 解析IP地址（支持域名/IP）
    if (inet_pton(AF_INET, ip, &serveraddr.sin_addr) <= 0) {
        struct hostent *h = gethostbyname(ip);
        if (h == nullptr) {
            logfile.write("gethostbyname failed for %s\n", ip);
            close(sockfd);
            return -1;
        }
        memcpy(&serveraddr.sin_addr, h->h_addr_list[0], h->h_length);
    }

    // 非阻塞connect
    int ret = connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (ret == 0) {
        // 立即连接成功
        logfile.write("connect to %s:%d success immediately\n", ip, port);
        return sockfd;
    } else if (errno == EINPROGRESS) {
        // 连接正在进行中，后续由epoll监听可写事件判断是否连接成功
        logfile.write("connect to %s:%d in progress\n", ip, port);
        return sockfd;
    } else {
        // 真正的连接失败
        logfile.write("connect to %s:%d failed: %s\n", ip, port, strerror(errno));
        close(sockfd);
        return -1;
    }
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("\n");
        printf("Using :./inetd logfile inifile\n\n");
        printf("Sample:/A/tools/bin/inted /A/log/tools/inetd.log /etc/inetd.conf\n\n");
        printf("        /project/tools/bin/procctl 5 /project/tools/bin/inetd /tmp/inetd.log /etc/inetd.conf\n\n");
        printf("本程序的功能是正向代理，如果用到了1024以下的端口，则必须由root用户启动。\n");
        printf("logfile 本程序运行的日志文件。\n");
        printf("inifile 路由参数配置文件。\n");
        return -1;
    }

    // 打开日志文件
    if (logfile.open(argv[1]) == false) {
        printf("打开日志文件失败（%s）。\n", argv[1]);
        return -1;
    }

    // 加载路由配置
    if (loadroute(argv[2]) == false) {
        logfile.write("loadroute failed.\n");
        return -1;
    }
    logfile.write("加载代理路由参数成功，共%d条规则。\n", vroute.size());

    // 初始化监听服务程序并且把listensocket存入容器
    for (auto &aa : vroute) {
        aa.listensocket = initserver(aa.inport);
        if (aa.listensocket < 0) {
            logfile.write("initserver(%d) failed,跳过该规则。\n", aa.inport);
            continue;
        }
        // 设置监听socket为非阻塞
        if (set_nonblock(aa.listensocket) < 0) {
            close(aa.listensocket);
            aa.listensocket = -1;
            logfile.write("set_nonblock for port %d failed\n", aa.inport);
            continue;
        }
        logfile.write("端口%d监听成功，fd=%d\n", aa.inport, aa.listensocket);
    }

    // 创建epoll句柄
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        logfile.write("epoll_create1 failed: %s\n", strerror(errno));
        return -1;
    }

    struct epoll_event ev; // 声明事件的数据结构。

    // 将监听socket添加到epoll（水平触发，监听读事件）
    for (auto &aa : vroute) {
        if (aa.listensocket < 0) continue;

        ev.events = EPOLLIN; // 只监控读事件
        ev.data.fd = aa.listensocket;
        // 把监听的socket的事件加入epollfd中
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, aa.listensocket, &ev) == -1) {
            logfile.write("epoll_ctl add listenfd %d failed: %s\n", aa.listensocket, strerror(errno));
            close(aa.listensocket);
            aa.listensocket = -1;
        }
    }

    // // 把定时器加入epoll的方法
    // int tfd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);   // 创建timerfd。
    // struct itimerspec timeout;                                // 定时时间的数据结构。
    // memset(&timeout,0,sizeof(struct itimerspec));
    // timeout.it_value.tv_sec = 10;                            // 定时时间为10秒。
    // timeout.it_value.tv_nsec = 0;
    // timerfd_settime(tfd,0,&timeout,0);                  // 开始计时。alarm(10)
    // ev.data.fd=tfd;                                                  // 为定时器准备事件。
    // ev.events=EPOLLIN;
    // epoll_ctl(epoll_fd,EPOLL_CTL_ADD,tfd,&ev);     // 把定时器fd加入epoll。

    struct epoll_event evs[MAX_EVENTS]; // 返回wait结果事件结构体
    logfile.write("epoll初始化完成，开始监听事件...\n");

    while (true) {
        // 等待事件，超时时间-1表示永久阻塞
        int nfds = epoll_wait(epoll_fd, evs, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                logfile.write("epoll_wait被信号中断，继续循环\n");
                continue; // 被信号中断，继续
            }
            logfile.write("epoll_wait failed: %s\n", strerror(errno));
            break;
        }

        // 遍历所有就绪事件
        for (int ii = 0; ii < nfds; ii++) {
            int cur_fd = evs[ii].data.fd;
            logfile.write("已发生事件的fd=%d，事件类型=%d\n", cur_fd, evs[ii].events);
            
            // //如果定时器时间已到,要更新进程心跳并且清空空闲的客户端的socket，这是常见的清理手段
            // if(evs[ii].data.fd==tfd){
            //     logfile.write("定时器时间已到。\n");
            //     timerfd_settime(tfd,0,&timeout,0);       // 重新开始计时。 等于alarm(10)
            //     //    pactive.uptatime();        // 1）更新进程心跳。

            //      //    // 2）清理空闲的客户端socket。
            //    for (int jj=0;jj<MAXSOCK;jj++)         // 可以把最大的socket记下来，这个循环不必遍历整个数组。
            //    {
            //        // 如果客户端socket空闲的时间超过80秒就关掉它。
            //        if ( (clientsocks[jj]>0) && ((time(0)-clientatime[jj])>10) )
            //        {
            //            logfile.write("client(%d,%d) timeout。\n",clientsocks[jj],clientsocks[clientsocks[jj]]);
            //            close(clientsocks[clientsocks[jj]]);
            //            close(clientsocks[jj]);  
            //            // 把数组中对端的socket置空，这一行代码和下一行代码的顺序不能乱。
            //            clientsocks[clientsocks[jj]]=0;
            //            // 把数组中本端的socket置空，这一行代码和上一行代码的顺序不能乱。
            //            clientsocks[jj]=0;
            //        }
            //    }
            // }

            // 标记是否是新连接事件
            bool is_new_conn = false;

            // 遍历路由规则，判断是否是监听socket的新连接事件
            for (int jj = 0; jj < vroute.size(); jj++) {
                if (cur_fd == vroute[jj].listensocket) {
                    is_new_conn = true;

                    // 循环accept，处理所有待接受的连接（边缘触发需循环）
                    while (true) {
                        struct sockaddr_in client;
                        socklen_t len = sizeof(client);
                        int srcsock = accept(vroute[jj].listensocket, (struct sockaddr *)&client, &len);
                        if (srcsock < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 没有更多待接受的连接
                                break;
                            } else {
                                logfile.write("accept failed: %s\n", strerror(errno));
                                break;
                            }
                        }

                        // 检查连接数是否超限
                        if (srcsock >= MAXSOCK) {
                            logfile.write("连接数已超过最大值%d，关闭新连接。\n", MAXSOCK);
                            close(srcsock);
                            break;
                        }

                        // 连接目标服务器
                        int dstsock = conntodst(vroute[jj].ipaddr, vroute[jj].outport);
                        if (dstsock < 0) {
                            logfile.write("连接目标服务器%s:%d失败，关闭客户端连接。\n", vroute[jj].ipaddr, vroute[jj].outport);
                            close(srcsock);
                            break;
                        }
                        if (dstsock >= MAXSOCK) {
                            logfile.write("连接数已超过最大值%d，关闭两端连接。\n", MAXSOCK);
                            close(srcsock);
                            close(dstsock);
                            break;
                        }

                        // 设置两端socket为非阻塞
                        set_nonblock(srcsock);
                        set_nonblock(dstsock);

                        // 维护socket映射关系
                        clientsocks[srcsock] = dstsock;
                        clientsocks[dstsock] = srcsock;

                        // 记录活动时间
                        time_t now = time(0);
                        clientatime[srcsock] = now;
                        clientatime[dstsock] = now;

                        // 为新连接的两个socket添加epoll读事件
                        ev.data.fd = srcsock;
                        ev.events = EPOLLIN | EPOLLRDHUP; // 读事件 + 连接关闭事件
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srcsock, &ev);

                        ev.data.fd = dstsock;
                        ev.events = EPOLLIN | EPOLLRDHUP;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dstsock, &ev);

                        logfile.write("接受新连接：端口%d，客户端fd=%d，目标端fd=%d\n", 
                                     vroute[jj].inport, srcsock, dstsock);
                    }
                    break;
                }
            }

            // 如果是新连接事件，跳过后续的数据处理逻辑
            if (is_new_conn) continue;

            // 处理数据收发/连接断开事件
            // 1. 先判断是否是连接关闭事件
            if (evs[ii].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                int peer_fd = clientsocks[cur_fd];
                logfile.write("连接断开：fd=%d，对端fd=%d\n", cur_fd, peer_fd);

                // 从epoll中删除fd
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, nullptr);
                if (peer_fd > 0 && peer_fd < MAXSOCK) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, peer_fd, nullptr);
                }

                // 关闭fd
                close(cur_fd);
                if (peer_fd > 0 && peer_fd < MAXSOCK) {
                    close(peer_fd);
                }

                // 清空映射关系
                if (peer_fd > 0 && peer_fd < MAXSOCK) {
                    clientsocks[peer_fd] = 0;
                }
                clientsocks[cur_fd] = 0;
                clientatime[cur_fd] = 0;
                if (peer_fd > 0 && peer_fd < MAXSOCK) {
                    clientatime[peer_fd] = 0;
                }
                continue;
            }

            // 2. 读取数据
            char buff[BUFFER_SIZE];
            ssize_t recv_len = recv(cur_fd, buff, sizeof(buff), 0);
            if (recv_len < 0) {
                // 非阻塞模式下无数据，正常返回
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                // 真正的读错误，关闭连接
                logfile.write("recv from fd %d failed: %s\n", cur_fd, strerror(errno));
                int peer_fd = clientsocks[cur_fd];
                close(cur_fd);
                if (peer_fd > 0 && peer_fd < MAXSOCK) close(peer_fd);
                clientsocks[cur_fd] = 0;
                if (peer_fd > 0 && peer_fd < MAXSOCK) clientsocks[peer_fd] = 0;
                continue;
            } else if (recv_len == 0) {
                // 客户端主动关闭连接
                logfile.write("fd %d 客户端主动关闭连接\n", cur_fd);
                int peer_fd = clientsocks[cur_fd];
                close(cur_fd);
                if (peer_fd > 0 && peer_fd < MAXSOCK) close(peer_fd);
                clientsocks[cur_fd] = 0;
                if (peer_fd > 0 && peer_fd < MAXSOCK) clientsocks[peer_fd] = 0;
                continue;
            }

            // 3. 转发数据到对端
            int peer_fd = clientsocks[cur_fd];
            if (peer_fd <= 0 || peer_fd >= MAXSOCK) {
                logfile.write("fd %d 找不到对端fd，关闭连接\n", cur_fd);
                close(cur_fd);
                clientsocks[cur_fd] = 0;
                continue;
            }

            // 非阻塞发送（循环发送确保数据发完）
            ssize_t send_len = 0;
            while (send_len < recv_len) {
                ssize_t ret = send(peer_fd, buff + send_len, recv_len - send_len, MSG_NOSIGNAL);
                if (ret < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 缓冲区满，稍后再发（简单处理：放弃本次剩余数据，实际可优化）
                        logfile.write("fd %d 发送缓冲区满，部分数据未发送\n", peer_fd);
                        break;
                    } else {
                        logfile.write("send to fd %d failed: %s\n", peer_fd, strerror(errno));
                        close(cur_fd);
                        close(peer_fd);
                        clientsocks[cur_fd] = 0;
                        clientsocks[peer_fd] = 0;
                        break;
                    }
                }
                send_len += ret;
            }

            if (send_len > 0) {
                logfile.write("从fd %d 转发 %zd 字节到fd %d\n", cur_fd, send_len, peer_fd);
                // 更新活动时间
                clientatime[cur_fd] = time(0);
                clientatime[peer_fd] = time(0);
            }
        }
    }

    // 清理资源
    close(epoll_fd);
    for (auto &aa : vroute) {
        if (aa.listensocket > 0) {
            close(aa.listensocket);
        }
    }
    return 0;
}

// 加载路由配置文件
bool loadroute(char *conf) {
    cifile ifile;
    ccmdstr cmdstr;
    if (ifile.open(conf) == false) {
        logfile.write("打开配置文件%s失败：%s\n", conf, strerror(errno));
        return false;
    }

    string buff;
    int line_num = 0;
    while (true) {
        line_num++;
        if (ifile.readline(buff) == false) {
            break; // 读取完毕
        }

        // 去掉注释
        size_t pos = buff.find("#");
        if (pos != string::npos) {
            buff.resize(pos);
        }

        // 去掉多余空格和换行
        replacestr(buff, "  ", " ", true);
        dellrstr(buff, ' ');

        // 空行跳过
        if (buff.empty()) {
            continue;
        }

        // 分割配置项
        cmdstr.splittocmd(buff, " ");
        if (cmdstr.size() != 3) {
            logfile.write("第%d行配置格式错误，需3个参数，实际%d个\n", line_num, cmdstr.size());
            continue;
        }

        st_route route;
        memset(&route, 0, sizeof(route));
        if (!cmdstr.getvalue(0, route.inport) || !cmdstr.getvalue(2, route.outport)) {
            logfile.write("第%d行端口号格式错误\n", line_num);
            continue;
        }
        cmdstr.getvalue(1, route.ipaddr, sizeof(route.ipaddr) - 1);
        route.listensocket = -1;

        vroute.push_back(route);
        logfile.write("加载第%d行规则：%d → %s:%d\n", line_num, route.inport, route.ipaddr, route.outport);
    }

    ifile.close();
    return !vroute.empty(); // 至少加载一条规则才算成功
}

// 初始化监听端口
int initserver(const int inport) {
    // 创建监听socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        logfile.write("创建监听socket失败（端口%d）：%s\n", inport, strerror(errno));
        return -1;
    }

    // 设置端口复用
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logfile.write("setsockopt失败（端口%d）：%s\n", inport, strerror(errno));
        close(listenfd);
        return -1;
    }

    // 绑定地址和端口
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网卡
    serveraddr.sin_port = htons(inport);

    // 修复类型转换错误：struct sockaddr* 而非 struct socketaddr*
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        logfile.write("bind失败（端口%d）：%s\n", inport, strerror(errno));
        close(listenfd);
        return -1;
    }

    // 开始监听
    if (listen(listenfd, 10) != 0) {
        logfile.write("listen失败（端口%d）：%s\n", inport, strerror(errno));
        close(listenfd);
        return -1;
    }

    return listenfd;
}