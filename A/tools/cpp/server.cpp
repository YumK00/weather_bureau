#include"mpublic.h"
using namespace std;
using namespace idc;

struct st_procinfo{
    int pid=0;
    char pname[51]={0};
    int timeout = 0;
    time_t atime = 0;//最后一次心跳时间

    st_procinfo(){}
    st_procinfo(const int id,const string&name,const int timeot,const long tm):pid(id),timeout(timeot),atime(tm){
        strncpy(pname,name.c_str(),50);
    }    

};
int m_shmid =-1;//共享内存id
st_procinfo * m_shm = nullptr;//指向共享内存地址的指针
int m_pos =-1;//共享内存内部的第x个地址


void EXIT(int sig);

int main(){
    signal(SIGTERM,EXIT);
    signal(SIGINT,EXIT);
    csemp semlock;
    if(semlock.init(getpid(0x5005))==false)
    {
        cout<<"创建信号量失败\n";
        EXIT(-1);
    }
    //返回共享内存id，参数是地址，大小，方式
    m_shmid = shmget(0x5005,1000*sizeof(st_procinfo),0666|IPC_CREAT);
    if(m_shmid==-1){
        perror("shmget failed");
        EXIT(-1);
    }

    semlock.wait();
    //把进程和共享内存连接
    m_shm = (st_procinfo *)shmat(m_shmid, NULL, 0);
    if (m_shm == (void *)-1) {
        perror("shmat failed");
       
        EXIT(-1);
    }
    //要保存的结构体
    st_procinfo procinfo(getpid(),"server1",30,time(nullptr));

    for(int i=0;i<1000;i++){
        if(m_shm[i].pid==procinfo.pid){
            m_pos=i;
            cout<<"找到旧位置"<<m_pos<<endl;
            break;
        }
    }
    if(m_pos==-1){
        for(int i=0;i<1000;i++){
            if(m_shm[i].pid==0){
                m_pos=i;
                cout<<"找到新位置"<<m_pos<<endl;
                break;
            }
        }
    }
    
    if(m_pos==-1){
        semlock.post();
        perror("共享空间已用完。\n");
        EXIT(-1);
    }
    
    memcpy(&m_shm[m_pos],&procinfo,sizeof(st_procinfo));
    semlock.post();
    //调试代码
    //  for(int i=0;i<1000;i++){
    //     if(m_shm[i].pid!=0){
            
    //         cout<<"i="<<i<<
    //         ",pid="<<m_shm[i].pid<<
    //         ",pname="<<m_shm[i].pname<<
    //         ",timeout="<<m_shm[i].timeout<<
    //         ",atime="<<m_shm[i].atime<<endl;
    //     }
    // }
    
    while(true){
        cout<<"服务正在运行中"<<endl;
        sleep(3);

        //更新心跳信息
        m_shm[m_pos].atime=time(nullptr);
    }

    return 0;
}

void EXIT(int sig){//但是只能捕获正常终止sigterm和sigint，如果不正常退出那就不能捕获
    cout<<"sig="<<sig<<endl;
    //分离共享内存
    if(m_pos!=-1){
        memset(m_shm+m_pos,0,sizeof(st_procinfo));
    }
    if (shmdt(m_shm) !=0) {
        perror("shmdt failed");
    }
    //删除共享内存
    // if (shmctl(m_shmid, IPC_RMID, NULL) == -1) {
    //         perror("shmctl删除失败");
    //     } else {
    //         printf("共享内存已删除\n");
    //     }
    exit(0);

}