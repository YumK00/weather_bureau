#include"mpublic.h"
using namespace idc;


int main(int argc,char*argv[]){
    if (argc != 2)
    {
        printf("\n");
        printf("Using:./checkproc logfilename\n");

        printf("Example:/A/tools/bin/procctl 10 /A/tools/bin/checkproc /A/log/tools/checkproc.log\n\n");

        printf("本程序用于检查后台服务程序是否超时，如果已超时，就终止它。\n");
        printf("注意：\n");
        printf("  1.本程序由procctl启动,运行周期建议为10秒。\n");
        printf("  2.为了避免被普通用户误杀,本程序应该用root用户启动。\n");
        printf("  3.如果要停止本程序,只能用killall -9 终止。\n\n\n");

        return -1;
    }
    // 忽略全部的信号和IO，不处理程序的退出信号。
    //closeioandsignal();
     // 打开日志文件。
     clogfile logfile;
     if(logfile.open(argv[1])==false){
        printf("logfile.open(%s) failed.\n",argv[1]);   return -1; 
     }
     // 创建/获取共享内存，键值为SHMKEYP，大小为MAXNUMP个st_procinfo结构体的大小。
    int shmid = 0;
    shmid = shmget(SHMKEYP,MAXNUMP*sizeof(st_procinfo),0666|IPC_CREAT);
    if(shmid==0){
        logfile.write("创建/获取共享内存(%x)失败。\n",SHMKEYP);
        return -1;
    }
     // 将共享内存连接到当前进程的地址空间。
    st_procinfo* shm=(st_procinfo*)shmat(shmid,0,0);//就是用这个指针指向共享空间的0号位置
     // 遍历共享内存中全部的记录，如果进程已超时，终止它。
    for(int ii =0;ii<MAXNUMP;ii++){
        if(shm[ii].pid==0)   continue;
        // logfile.write("ii=%d,pid=%d,pname=%s,timeout=%d,atime=%d\n",\
        //               ii,shm[ii].pid,shm[ii].pname,shm[ii].timeout,shm[ii].atime);
        time_t now = time(nullptr);
        if(now-shm[ii].atime<shm[ii].timeout) continue;
        else{
            logfile.write("进程pid=%d(%x)超时",shm[ii].pid,shm[ii].pname);
            kill(shm[ii].pid,SIGTERM);
        }
    }

     // 把共享内存从当前进程中分离。
     shmdt(shm);
    return 0;
}