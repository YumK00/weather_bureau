#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include<iostream>
using namespace std;

#include <fcntl.h>
#include <sys/epoll.h> 
#include <poll.h>  // poll模型核心头文件，必包含

//设置成非阻塞状态  
int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}




int main(int argc,char* argv[]){
    if(argc!=3){
        cout<<"./kehu 192.168.5.5 5010"<<endl;
        return 0;
    }
    int connfd = socket(AF_INET,SOCK_STREAM,0);

    set_nonblock(connfd);  // 关键步骤！

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(sockaddr_in));
    addr.sin_family=PF_INET;
    addr.sin_port=htons(atoi(argv[2]));
    addr.sin_addr.s_addr=inet_addr(argv[1]);

    int ret = connect(connfd,(struct sockaddr*)&addr,sizeof(addr));
    // ret == 0：连接立即成功（本地连接或非常快的网络）
    // ret == -1 && errno == EINPROGRESS：连接正在进行中（这是正常情况）
    // ret == -1 && errno != EINPROGRESS：连接立即失败
    if(ret == -1 && errno != EINPROGRESS){
        printf("connect(%s:%s) failed.\n",argv[1],argv[2]); close(connfd);  return -1;
    }
    pollfd fds;
    fds.fd= connfd;
    fds.events=POLLOUT;
    poll(&fds,1,-1);
    if(fds.revents & POLLOUT){  // 检查是否可写
    // 再排除错误标志
    if(!(fds.revents & (POLLERR | POLLHUP | POLLNVAL))){
        cout<<"connect success.\n";  // ✅ 真正的成功
    } else {
        cout<<"connect failed (with error flags).\n";
    }
}


    return 0;
    char buf[1024];
    for(int i=0;i<100;i++){
        memset(&buf,0,sizeof(buf));
        printf("please input:"); scanf("%s",buf);
        if (send(connfd,buf,strlen(buf),0) <=0)
        { 
            printf("write() failed.\n");  close(connfd);  return -1;
        }
        memset(buf,0,sizeof(buf));
        if (recv(connfd,buf,sizeof(buf),0) <=0) 
        { 
            printf("read() failed.\n");  close(connfd);  return -1;
        }
        printf("recv:%s\n",buf);
    }
    return 0;
}