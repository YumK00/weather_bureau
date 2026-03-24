#include"mpublic.h"
using namespace idc;

ctcpserver tcpserver;
clogfile logfile;
string sendbuffer,recebuffer;

void FatherEXIT(int sig);
void ChildEXIT(int sig);


//接收客户端传来的信息所用的结构体
struct st_srg{
    int    clienttype;                // 客户端类型，1-上传文件；2-下载文件，本程序固定填1。
    char ip[31];                       // 服务端的IP地址。
    int    port;                        // 服务端的端口。
    char clientpath[256];       // 本地文件存放的根目录。 /data /data/aaa /data/bbb
    int    ptype;                      // 文件上传成功后本地文件的处理方式：1-删除文件；2-移动到备份目录。
    char clientpathbak[256]; // 文件成功上传后，本地文件备份的根目录，当ptype==2时有效。
    bool andchild;                 // 是否上传clientpath目录下各级子目录的文件，true-是；false-否。
    char matchname[256];    // 待上传文件名的匹配规则，如"*.TXT,*.XML"。
    char srvpath[256];           // 服务端文件存放的根目录。/data1 /data1/aaa /data1/bbb
    int    timetvl;                    // 扫描本地目录文件的时间间隔（执行文件上传任务的时间间隔），单位：秒。 
    int    timeout;                  // 进程心跳的超时时间。
    char pname[51];             // 进程名，建议用"tcpputfiles_后缀"的方式。

}starg;

bool clientlogin();
void recvfilesmain();
bool recefile(const string& serverfilename,const int filesize,const string mtime);

int main(int argc,char*argv[]){
    if(argc!=3){
        cout<<"SAMPLE:./fileserver 5005 /A/log/tools/fileserver.log\n";
        return -1;
    }
    //closeioandsignal(true);
    signal(SIGINT,FatherEXIT);
    signal(SIGTERM,FatherEXIT);

    if(logfile.open(argv[2])==false){
        printf("logfile.open(%s) failed.\n",argv[2]);
        return -1;
    }
    if(tcpserver.initserver(atoi(argv[1]))==false){
        logfile.write("tcpserver.initserver(%s) failed.\n",argv[1]);
        return -1;
    }
    while(true){
        if(tcpserver.accept()==false){
            logfile.write("tcpserver.accept() failed.\n");
            FatherEXIT(-1);
        }
        logfile.write("客户端(%s)已连接。\n",tcpserver.getid());
        if(fork()>0){
            tcpserver.closeconnectfd();
            continue;
        }
        signal(SIGINT,ChildEXIT);
        signal(SIGTERM,ChildEXIT);
        tcpserver.closelistenfd();
        //处理业务
        if(clientlogin()==false){
            logfile.write("clientlogin failed.\n");
            ChildEXIT(-1);
        }
        if(starg.clienttype==1)
            recvfilesmain();
        // if(starg.clienttype==2)
        //     sendfilesmain();

        recvfilesmain();
        ChildEXIT(0);
    }
    
    return 0;
}

void recvfilesmain()
{
    while(true){
        if(tcpserver.read(recebuffer,20)==false){
            logfile.write("tcpserver.read() failed.\n");
            return;
        }
        if(recebuffer=="<activetest>ok</activetest>"){
            sendbuffer="ok";
            if(tcpserver.write(sendbuffer)==false){
                logfile.write("tcpserver.write() failed.\n"); 
                return;
            }
        }
        if(recebuffer.find("filename")!=string::npos){
            string clientfilename;
            int filesize;
            string mtime;
            getxmlbuffer(recebuffer,"filename",clientfilename);
            getxmlbuffer(recebuffer,"size",filesize);
            getxmlbuffer(recebuffer,"mtime",mtime);

            //文件处理
            string serverfilename;
            serverfilename = clientfilename;
            replacestr(serverfilename,starg.clientpath,starg.srvpath,false);
            logfile.write("receive file:%s(%d bite) to %s\n",clientfilename.c_str(),filesize,serverfilename.c_str());
            if(recefile(serverfilename,filesize,mtime)==false){
                logfile.write("recefile failed.\n");
                return;
            }

            sformat(sendbuffer,"<filename>%s</filename><result>ok<result>\n",clientfilename);
            if(tcpserver.write(sendbuffer)==false){
                logfile.write("tcpserver.write(%s)failed.\n",clientfilename);
                return ;
            }
        }
    }
}

bool recefile(const string &serverfilename, const int filesize, const string mtime)
{
    cofile ofile;
    int haswrited=0;
    string buff[1000];
    int curwrite;
    if(ofile.open(serverfilename,true,ios::out|ios::binary)==false){
        logfile.write("ofile.write failed.\n");
        return false;
    }
    while(true){
        if(filesize-haswrited>1000) curwrite=1000;
        else    curwrite=filesize-haswrited;
        if(ofile.write(buff,curwrite)==false){
            logfile.write("ofile.write failed.\n");
            return false;
        }
        haswrited+=curwrite;
        if(haswrited==filesize) break;
    }
    return true;
}

bool clientlogin()
{
    if(tcpserver.read(recebuffer,20)==false){
        logfile.write("read failed.\n");
        return false;
    }
    logfile.write("recebuffer=%s\n",recebuffer.c_str());
    getxmlbuffer(recebuffer,"clientpath",starg.clientpath);
    getxmlbuffer(recebuffer,"clienttype",starg.clienttype);
    getxmlbuffer(recebuffer,"srvpath",starg.srvpath);
    if(starg.clienttype!=1&&starg.clienttype!=2){
        sendbuffer="failed";
    }
    else{
        sendbuffer="ok";
    }
    if(tcpserver.write(sendbuffer)==false){
        logfile.write("write failed.\n");
        return false;
    }

    logfile.write("%s login %s.\n",tcpserver.getid(),sendbuffer.c_str());
    return true;
}

void FatherEXIT(int sig)
{
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断。
    signal(SIGINT,SIG_IGN); signal(SIGTERM,SIG_IGN);

    logfile.write("父进程退出,sig=%d。\n",sig);
    tcpserver.closelistenfd();
    kill(0,15);
    exit(0);
}

void ChildEXIT(int sig)
{
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断。
    signal(SIGINT,SIG_IGN); signal(SIGTERM,SIG_IGN);
    logfile.write("子进程退出,sig=%d。\n",sig);
    tcpserver.closeconnectfd();
    exit(0);
}