#include<mpublic.h>
// #include<signal.h>
// #include <unistd.h>
int main(int argc,char* argv[]){
    if(argc<3){
        printf("Using:./procctl timetvl program argv ...\n");
        //printf("Example:/project/tools/bin/procctl 10 /usr/bin/tar zcvf /tmp/tmp.tgz /usr/include\n");
  	    printf("Example:/A/tools/bin/procctl 60 /A/idc/bin/crtsurfdata /A/idc/ini/stcode.ini /A/idc/observedata /A/log/idc/crtsurfdata.log csv,xml,json\n");
        printf("本程序是服务程序的调度程序，周期性启动服务程序或shell脚本。\n");
        printf("timetvl 运行周期，单位：秒。\n");
        printf("被调度的程序运行结束后，在timetvl秒后会被procctl重新启动。\n");
        printf("如果被调度的程序是周期性的任务，timetvl设置为运行周期。\n");
        printf("如果被调度的程序是常驻内存的服务程序，timetvl设置小于5秒。\n"); 
        printf("program 被调度的程序名，必须使用全路径。\n");
        printf("...   被调度的程序的参数。\n");
        printf("注意，本程序不会被kill杀死，但可以用kill -9强行杀死。\n\n\n");

        return -1; 
    }

    for(int ii=0;ii<64;ii++){
        signal(ii,SIG_IGN);//关闭标准输入输出
        close(ii);//关闭文件描述符
    }
    if(fork()!=0)   exit(0);//关闭父进程，子进程挂在pid1下不受控制
    signal(SIGCHLD,SIG_DFL);//把子进程的信号打开，因为后面父进程要wait子进程的返回值

    char* gram[argc];
    for(int ii=2;ii<argc;ii++){
        gram[ii-2]=argv[ii];
    }
    gram[argc-2]=nullptr;
    while(true){
        if(fork()==0){
            execv(argv[2],gram);
            exit(0); 
        }
        else{
            int status;
            ::wait(&status);
            sleep(atoi(argv[1]));
        }
    }
    return 0;
}