#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
using namespace std;

#define MAX_CLIENTS 2048  // 最大客户端数量

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " 5010" << endl;
        return 0;
    }
    
    // 1. 创建监听socket（与select版本相同）
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket() failed");
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        close(listenfd);
        return -1;
    }
    
    if (listen(listenfd, 5) < 0) {
        perror("listen() failed");
        close(listenfd);
        return -1;
    }
    
    cout << "Server listening on port " << argv[1] << "..." << endl;
    
    // 2. 初始化pollfd数组
    struct pollfd fds[MAX_CLIENTS];
    
    // 全部初始化为无效
    for (int i = 0; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;        // -1表示这个位置未使用
        fds[i].events = 0;     // 不监视任何事件
        fds[i].revents = 0;    // 清除返回事件
    }
    
    // 3. 添加监听socket到数组的第一个位置
    fds[0].fd = listenfd;
    fds[0].events = POLLIN;    // 监视可读事件（新连接）
    
    int nfds = 1;              // 当前有效的pollfd数量（包括监听socket）
    int current_size = 1;      // 当前使用的数组位置数
    
    // 4. 主循环
    while (true) {
        // 调用poll，等待事件（10秒超时）
        int timeout = 10 * 1000;  // 10秒，单位是毫秒
        int ret = poll(fds, nfds, timeout);
        
        if (ret < 0) {
            perror("poll() failed");
            break;
        }
        
        if (ret == 0) {
            cout << "poll() timeout" << endl;
            continue;
        }
        
        // 5. 检查所有有效的pollfd
        for (int i = 0; i < nfds; i++) {
            // 跳过无效的fd
            if (fds[i].fd < 0) continue;
            
            // 检查是否有事件发生
            if (fds[i].revents == 0) continue;
            
            // 6. 处理监听socket（新连接）
            if (fds[i].fd == listenfd) {
                if (fds[i].revents & POLLIN) {
                    struct sockaddr_in clientaddr;
                    socklen_t clientlen = sizeof(clientaddr);
                    int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
                    
                    if (connfd < 0) {
                        perror("accept() failed");
                        continue;
                    }
                    
                    cout << "accept client(socket=" << connfd << ") ok." << endl;
                    
                    // 找到空闲位置存放新连接
                    bool added = false;
                    for (int j = 1; j < MAX_CLIENTS; j++) {
                        if (fds[j].fd == -1) {  // 找到空闲位置
                            fds[j].fd = connfd;
                            fds[j].events = POLLIN;  // 监视客户端数据
                            fds[j].revents = 0;
                            
                            // 更新nfds
                            if (j >= nfds) {
                                nfds = j + 1;
                            }
                            
                            current_size++;
                            added = true;
                            break;
                        }
                    }
                    
                    if (!added) {
                        cout << "Too many clients, connection refused." << endl;
                        close(connfd);
                    }
                }
            }
            // 7. 处理客户端socket（数据到达）
            else {
                if (fds[i].revents & POLLIN) {
                    char buf[1024];
                    memset(buf, 0, sizeof(buf));
                    
                    ssize_t n = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                    
                    if (n <= 0) {
                        // 客户端断开连接或出错
                        if (n == 0) {
                            cout << "client(socket=" << fds[i].fd << ") disconnected." << endl;
                        } else {
                            perror("recv() failed");
                        }
                        
                        close(fds[i].fd);
                        fds[i].fd = -1;      // 标记为空闲
                        fds[i].events = 0;
                        fds[i].revents = 0;
                        current_size--;
                        
                        // 如果关闭的是最后一个有效的fd，调整nfds
                        if (i == nfds - 1) {
                            // 找到新的最大索引
                            for (int j = nfds - 2; j >= 0; j--) {
                                if (fds[j].fd != -1) {
                                    nfds = j + 1;
                                    break;
                                }
                            }
                        }
                    } else {
                        buf[n] = '\0';
                        cout << "recv(socket=" << fds[i].fd << "):" << buf << endl;
                        
                        // 原样返回数据
                        send(fds[i].fd, buf, n, 0);
                    }
                }
                
                // 可以处理其他事件，如POLLOUT、POLLERR等
                if (fds[i].revents & POLLERR) {
                    cout << "Error on socket " << fds[i].fd << endl;
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                    current_size--;
                }
            }
            
            // 8. 清除已处理的事件
            fds[i].revents = 0;
        }
        
        // 可选：定期输出状态信息
        static int loop_count = 0;
        if (++loop_count % 100 == 0) {
            cout << "Current connections: " << current_size - 1 
                 << ", nfds=" << nfds << endl;
        }
    }
    
    // 9. 清理（关闭所有socket）
    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0) {
            close(fds[i].fd);
        }
    }
    
    return 0;
}