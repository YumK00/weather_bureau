#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
using namespace std;
#include <iostream>


int main(int argc,char* argv[]){
    if(argc!=2){
        cout<<"/A/public/demo/kehuduan.cpp 5010"<<endl;
        return 0;
    }
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(atoi(argv[1]));
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    bind(listenfd,(struct sockaddr*)&addr,sizeof(addr));
    listen(listenfd,5);
    cout<<"connfd="<<listenfd<<endl;
    int listensock = listenfd;
    if (listensock<0){
        cout<<"connfd="<<listenfd<<endl;
        return 0;
    }
    
    fd_set readfds;//本质是一个bitmap
    FD_ZERO(&readfds);
    FD_SET(listensock,&readfds);
    int maxfd=listensock;

    while(true){
        struct timeval timeout;     
        timeout.tv_sec=10;
        timeout.tv_usec=0;

        fd_set tmpfds=readfds;
        int ds=select(maxfd+1,&tmpfds,NULL,NULL,0);
        
        if (ds<0){
            perror("select() failed"); break;
        }
        if (ds==0){
            printf("select() timeout.\n"); continue;
        }
        
        for(int eventfd=0;eventfd<=maxfd;eventfd++){ 
            if(FD_ISSET(eventfd,&tmpfds)==0) continue;

            if(eventfd==listensock){  // 注意：这里用listensock
                struct sockaddr_in clientaddr;
                socklen_t clientaddr_len = sizeof(clientaddr); 
                int connfd= accept(listenfd,(struct sockaddr*)&clientaddr,&clientaddr_len);
                if (connfd < 0) { perror("accept() failed"); continue; }
                printf ("accept client(socket=%d) ok.\n",connfd);
                FD_SET(connfd,&readfds);
                if (maxfd<connfd) maxfd=connfd;
            }
            else{
                char buf[1024];
                memset(&buf,0,sizeof(buf));
                if(recv(eventfd,&buf,sizeof(buf),0)<=0){
                    printf("client(eventfd=%d) disconnected.\n",eventfd);
                    close(eventfd);
                    FD_CLR(eventfd,&readfds);
                    
                    // 修正maxfd重新计算逻辑
                    if (eventfd == maxfd){
                        // 从高到低查找当前最大的文件描述符
                        for (int ii=maxfd-1;ii>=0;ii--){
                            if (FD_ISSET(ii,&readfds)){
                                maxfd = ii;
                                break;
                            }
                        }
                    }
                }
                else{
                    printf("recv(eventfd=%d):%s\n",eventfd,buf);
                    send(eventfd,buf,strlen(buf),0);
                }
            }
        }
    }    
    return 0;
}
/*int main(int argc,char* argv[]){
    if(argc!=2){
        cout<<"/A/public/demo/kehuduan.cpp 5000"<<endl;
        return 0;
    }
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(sockaddr_in));
    addr.sin_family=PF_INET;
    addr.sin_port=htons(atoi(argv[1]));
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    bind(listenfd,(struct sockaddr*)&addr,sizeof(addr));
    listen(listenfd,5);
    int listensock = listenfd;
    if (listensock<0){
        cout<<"connfd="<<listenfd<<endl;
        return 0;
    }
    
    fd_set readfds;                         // 需要监视读事件的socket的集合，大小为16字节（1024位）的bitmap。
    FD_ZERO(&readfds);                // 初始化readfds，把bitmap的每一位都置为0。
    FD_SET(listensock,&readfds);  // 把服务端用于监听的socket加入readfds。
    int maxfd=listensock;              // readfds中socket的最大值。

    while(true){
        // 用于表示超时时间的结构体。
        struct timeval timeout;     
        timeout.tv_sec=10;        // 秒
        timeout.tv_usec=0;        // 微秒。

        // 在select()函数中，会修改bitmap，所以，要把readfds复制一份给tmpfds，再把tmpfds传给select()。
        fd_set tmpfds=readfds;      
        int ds=select(maxfd+1,&tmpfds,NULL,NULL,0);//监控0-maxfd+1d的所有文件描述符的事件
        // 如果infds<0，表示调用select()失败。
        if (ds<0)
        {
            perror("select() failed"); break;
        }
        // 如果infds==0，表示select()超时。
        if (ds==0)
        {
            printf("select() timeout.\n"); continue;
        }
        for(int eventfd=0;eventfd<=maxfd;eventfd++){
            if(FD_ISSET(eventfd,&tmpfds)==0)    continue;

            if(eventfd==listenfd){
                struct sockaddr_in clientaddr;
                int connfd= accept(listenfd,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
                if (connfd < 0) { perror("accept() failed"); continue; }
                printf ("accept client(socket=%d) ok.\n",connfd);
                FD_SET(connfd,&readfds);
                if (maxfd<connfd) maxfd=connfd;
            }
            else{
                char buf[1024];
                memset(&buf,0,sizeof(buf));
                if(recv(eventfd,&buf,sizeof(buf),0)<=0){
                    printf("client(eventfd=%d) disconnected.\n",eventfd);
                    close(eventfd);
                    FD_CLR(eventfd,&readfds); 
                    if (eventfd == maxfd)              // 重新计算maxfd的值，注意，只有当eventfd==maxfd时才需要计算。
                    {
                        for (int ii=maxfd;ii>0;ii--)    // 从后面往前找。
                        {
                            if (FD_ISSET(ii,&readfds))
                            {
                                maxfd = ii; break;
                            }
                        }
                    }
                }
                else{// 如果客户端有报文发过来。
                    printf("recv(eventfd=%d):%s\n",eventfd,buf);

                    // 把接收到的报文内容原封不动的发回去。
                    send(eventfd,buf,strlen(buf),0);
                }
            }
        }
    }    
    return 0;
}*/