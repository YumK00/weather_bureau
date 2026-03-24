// epoll客户端（使用epoll处理非阻塞connect）
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
using namespace std;

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

// 设置文件描述符为非阻塞
int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return -1;
    }
    
    // 1. 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return -1;
    }
    
    // 设置为非阻塞
    set_nonblock(sockfd);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    // 2. 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(sockfd);
        return -1;
    }
    
    // 3. 发起非阻塞连接
    int ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    if (ret == 0) {
        // 立即连接成功
        cout << "Connect immediate success." << endl;
    } 
    else if (ret < 0 && errno == EINPROGRESS) {
        // 连接正在进行中，使用epoll等待连接完成
        cout << "Connect in progress, using epoll to wait..." << endl;
        
        // 添加sockfd到epoll，监听可写事件（连接成功时会触发可写）
        struct epoll_event ev;
        ev.events = EPOLLOUT;  // 等待可写事件（连接成功）
        ev.data.fd = sockfd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
            perror("epoll_ctl failed");
            close(sockfd);
            close(epoll_fd);
            return -1;
        }
        
        // 等待连接完成
        struct epoll_event events[MAX_EVENTS];
        int timeout = 5000;  // 5秒超时
        int nfds = 0;
        
        while (timeout > 0) {
            nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
            
            if (nfds < 0) {
                if (errno == EINTR) continue;
                perror("epoll_wait failed");
                break;
            }
            
            if (nfds == 0) {
                cout << "Connect timeout." << endl;
                close(sockfd);
                close(epoll_fd);
                return -1;
            }
            
            // 检查事件
            for (int i = 0; i < nfds; i++) {
                if (events[i].data.fd == sockfd && (events[i].events & EPOLLOUT)) {
                    // 连接完成，检查是否成功
                    int error = 0;
                    socklen_t len = sizeof(error);
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
                    
                    if (error == 0) {
                        cout << "Connect success via epoll." << endl;
                        // 从epoll中移除连接完成监听
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
                        goto connect_success;
                    } else {
                        cout << "Connect failed: " << strerror(error) << endl;
                        close(sockfd);
                        close(epoll_fd);
                        return -1;
                    }
                }
            }
            
            timeout -= 100;  // 简单模拟超时递减
            usleep(100000);  // 100ms
        }
        
        cout << "Connect timeout after loop." << endl;
        close(sockfd);
        close(epoll_fd);
        return -1;
    } 
    else {
        // 连接立即失败
        cout << "Connect failed immediately: " << strerror(errno) << endl;
        close(sockfd);
        close(epoll_fd);
        return -1;
    }
    
connect_success:
    // 4. 连接成功，重新设置epoll监听读写事件
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // 边缘触发模式
    ev.data.fd = sockfd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl for read failed");
        close(sockfd);
        close(epoll_fd);
        return -1;
    }
    
    // 5. 添加标准输入到epoll
    set_nonblock(STDIN_FILENO);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = STDIN_FILENO;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        perror("epoll_ctl for stdin failed");
        close(sockfd);
        close(epoll_fd);
        return -1;
    }
    
    cout << "\n=== Client Started ===\n";
    cout << "Type messages and press Enter to send.\n";
    cout << "Type 'quit' to exit.\n" << endl;
    
    // 6. 主循环：处理epoll事件
    struct epoll_event events[MAX_EVENTS];
    bool running = true;
    
    while (running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait failed");
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            // 6.1 处理标准输入（用户输入）
            if (events[i].data.fd == STDIN_FILENO) {
                char input_buf[BUFFER_SIZE];
                memset(input_buf, 0, sizeof(input_buf));
                
                // 边缘触发：一次性读取所有输入
                ssize_t n = read(STDIN_FILENO, input_buf, sizeof(input_buf) - 1);
                if (n > 0) {
                    input_buf[n] = '\0';
                    
                    // 去掉换行符
                    if (input_buf[n-1] == '\n') {
                        input_buf[n-1] = '\0';
                    }
                    
                    // 检查是否退出
                    if (strcmp(input_buf, "quit") == 0) {
                        cout << "Exiting..." << endl;
                        running = false;
                        break;
                    }
                    
                    // 发送数据到服务器
                    ssize_t sent = send(sockfd, input_buf, strlen(input_buf), 0);
                    if (sent <= 0) {
                        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                            // 发送缓冲区满，可以稍后重试
                            cout << "Send buffer full, message may be lost." << endl;
                        } else {
                            cout << "Send failed: " << strerror(errno) << endl;
                            running = false;
                            break;
                        }
                    } else {
                        cout << "Sent: " << input_buf << endl;
                    }
                }
            }
            // 6.2 处理socket数据（服务器回应）
            else if (events[i].data.fd == sockfd) {
                // 检查连接是否关闭
                if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    cout << "Server closed connection." << endl;
                    running = false;
                    break;
                }
                
                // 处理可读事件（边缘触发：必须一次性读取所有数据）
                if (events[i].events & EPOLLIN) {
                    char recv_buf[BUFFER_SIZE];
                    
                    // 边缘触发：循环读取直到EAGAIN
                    while (true) {
                        memset(recv_buf, 0, sizeof(recv_buf));
                        ssize_t n = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0);
                        
                        if (n > 0) {
                            recv_buf[n] = '\0';
                            cout << "Received from server: " << recv_buf << endl;
                        } 
                        else if (n == 0) {
                            cout << "Server closed connection." << endl;
                            running = false;
                            break;
                        }
                        else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 数据已读取完毕
                                break;
                            } else {
                                cout << "Receive failed: " << strerror(errno) << endl;
                                running = false;
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
    close(sockfd);
    
    cout << "Client exit." << endl;
    return 0;
}