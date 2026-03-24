#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>  // ✅ epoll头文件
using namespace std;

#define MAX_EVENTS 1024

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "./fuwepoll 5010" << endl;
        return 0;
    }
    
    // 1. 创建监听socket（和你代码完全一样）
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listenfd, 5);
    
    cout << "Server listening on port " << argv[1] << "..." << endl;
    
    // 2. 创建epoll实例（epoll特有的第一步）
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        return -1;
    }
    
    // 3. 将监听socket添加到epoll（相当于select的FD_SET）
    struct epoll_event ev;
    ev.events = EPOLLIN;        // 监视可读事件（新连接）
    ev.data.fd = listenfd;      // 保存文件描述符

    //epoll_ctl:向 epoll 实例添加、修改或删除监控的文件描述符。
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listenfd");
        return -1;
    }
    
    // 4. 准备事件数组（epoll_wait返回的就绪事件放在这里）
    struct epoll_event events[MAX_EVENTS];
    
    while (true) {
        // 5. 等待事件（相当于select/poll的调用）
        // 超时10秒：10 * 1000 = 10000毫秒
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nfds < 0) {
            perror("epoll_wait failed");
            break;
        }
        
        if (nfds == 0) {
            cout << "epoll_wait timeout (10 seconds)" << endl;
            continue;
        }
        
        // 6. 处理所有就绪的事件
        // 注意：这里只需要遍历nfds个事件，不是2048或1024个！
        for (int i = 0; i < nfds; i++) {
            // 6.1 处理监听socket（新连接）
            if (events[i].data.fd == listenfd) {
                struct sockaddr_in clientaddr;
                socklen_t clientlen = sizeof(clientaddr);
                int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
                
                if (connfd < 0) {
                    perror("accept() failed");
                    continue;
                }
                
                cout << "accept client(socket=" << connfd << ") ok." << endl;
                
                // 将新连接添加到epoll（相当于select的FD_SET）
                struct epoll_event client_ev;
                client_ev.events = EPOLLIN;        // 监视客户端数据可读
                client_ev.data.fd = connfd;        // 保存客户端fd
                
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &client_ev) == -1) {
                    perror("epoll_ctl: connfd");
                    close(connfd);
                }
            }
            // 6.2 处理客户端socket（数据到达）
            else {
                int client_fd = events[i].data.fd;
                char buf[1024];
                memset(buf, 0, sizeof(buf));
                
                // 接收数据
                ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
                
                if (n <= 0) {
                    // 客户端断开连接
                    if (n == 0) {
                        cout << "client(socket=" << client_fd << ") disconnected." << endl;
                    } else {
                        perror("recv failed");
                    }
                    
                    // 从epoll中移除（相当于select的FD_CLR）
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                } else {
                    buf[n] = '\0';
                    cout << "recv(socket=" << client_fd << "):" << buf << endl;
                    
                    // 原样返回数据（echo功能）
                    send(client_fd, buf, n, 0);
                }
            }
        }
    }
    
    // 7. 清理资源
    close(epoll_fd);
    close(listenfd);
    
    return 0;
}