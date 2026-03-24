//epoll边缘触发服务器
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
using namespace std;

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096  // 增大缓冲区

// 设置文件描述符为非阻塞
int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return 0;
    }
    
    // 1. 创建监听socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket failed");
        return -1;
    }
    
    // 设置端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    set_nonblock(listenfd);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(listenfd);
        return -1;
    }
    
    if (listen(listenfd, 5) < 0) {
        perror("listen failed");
        close(listenfd);
        return -1;
    }
    
    cout << "Server listening on port " << argv[1] << " (EPOLLET mode)..." << endl;
    
    // 2. 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(listenfd);
        return -1;
    }
    
    // 3. 将监听socket添加到epoll（边缘触发模式）
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // 边缘触发模式
    ev.data.fd = listenfd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listenfd");
        close(listenfd);
        close(epoll_fd);
        return -1;
    }
    
    // 4. 准备事件数组
    struct epoll_event events[MAX_EVENTS];
    
    while (true) {
        // 5. 等待事件
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nfds < 0) {
            if (errno == EINTR) continue;  // 被信号中断，继续
            perror("epoll_wait failed");
            break;
        }
        
        // 6. 处理所有就绪的事件
        for (int i = 0; i < nfds; i++) {
            // 6.1 处理监听socket（新连接）
            if (events[i].data.fd == listenfd) {
                // 边缘触发：必须一次性accept所有连接
                while (true) {
                    struct sockaddr_in clientaddr;
                    socklen_t clientlen = sizeof(clientaddr);
                    int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
                    
                    if (connfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 已accept所有连接
                            break;
                        } else {
                            perror("accept failed");
                            break;
                        }
                    }
                    
                    // 设置新连接为非阻塞
                    set_nonblock(connfd);
                    
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip, sizeof(client_ip));
                    cout << "New connection from " << client_ip << ":" << ntohs(clientaddr.sin_port)
                         << " (socket=" << connfd << ")" << endl;
                    
                    // 将新连接添加到epoll（边缘触发模式）
                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;  // 边缘触发 + 对端关闭检测
                    client_ev.data.fd = connfd;
                    
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &client_ev) == -1) {
                        perror("epoll_ctl: connfd");
                        close(connfd);
                    }
                }
            }
            // 6.2 处理客户端socket（数据到达）
            else {
                int client_fd = events[i].data.fd;
                
                // 检查连接是否关闭
                if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    cout << "Client(socket=" << client_fd << ") disconnected." << endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    continue;
                }
                
                // 处理可读事件（边缘触发：必须一次性读取所有数据）
                if (events[i].events & EPOLLIN) {
                    // 边缘触发：循环读取直到EAGAIN
                    while (true) {
                        char buf[BUFFER_SIZE];
                        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
                        
                        if (n > 0) {
                            buf[n] = '\0';
                            cout << "recv(socket=" << client_fd << ", len=" << n << "): " << buf << endl;
                            
                            // 回显数据
                            ssize_t sent = 0;
                            while (sent < n) {
                                ssize_t nsend = send(client_fd, buf + sent, n - sent, 0);
                                if (nsend < 0) {
                                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                        // 可以稍后重试，但简单起见我们直接跳出
                                        break;
                                    } else {
                                        perror("send failed");
                                        break;
                                    }
                                }
                                sent += nsend;
                            }
                        } 
                        else if (n == 0) {
                            // 客户端正常关闭
                            cout << "Client(socket=" << client_fd << ") closed connection." << endl;
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            close(client_fd);
                            break;
                        }
                        else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 数据已读取完毕（边缘触发的关键）
                                break;
                            } else {
                                // 读取错误
                                perror("recv failed");
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                                close(client_fd);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 7. 清理资源
    close(epoll_fd);
    close(listenfd);
    
    return 0;
}