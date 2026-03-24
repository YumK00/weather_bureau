#include"mpublic.h"
using namespace idc;

ctcpclient tcpclient;
clogfile logfile;
string sendbuffer,recebuffer;

void EXIT(int sig);
void help();
bool activetest();
bool xmltoarg(const char * xml);
bool login(const char* argv);
bool tcpputfiles();
bool ackmessage(const string& buffer);
bool sendfile(const string& filename,const int filesize);

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

int main(int argc,char* argv[]){
    if(argc!=3){
        help();
        return -1;
    }
    //closeioandsignal(true);
    signal(SIGINT,SIG_IGN);
    signal(SIGTERM,SIG_IGN);

    if(logfile.open(argv[1])==false){
        cout<<"logfile.open(%s) failed.\n",argv[1];
        return -1;
    }
    if(xmltoarg(argv[2])==false){
        logfile.write("xmltolog failed,exit..\n");
        return -1;
    }

    if(tcpclient.connect(starg.ip,starg.port)==false){
        logfile.write("tcpclient.connect(%s,%s) failed.\n",starg.ip,starg.port); 
        EXIT(-1);
    }

    if(login(argv[2])==false){
        logfile.write("login failed\n");
        EXIT(-1);
    }
    if(tcpputfiles()==false){
            logfile.write("tcpputfiles failed.\n");
            EXIT(-1);
        }
    while(true){ 
        
        sleep(starg.timetvl);
        activetest();
    }

    return 0;
}


void help()
{
    printf("SAMPLE:./tcpputfiles /A/log/tools/tcpputfiles.log "\
        "\"<ip>192.168.5.5</ip>"\
        "<port>5005</port>"\
        "<clientpath>/A/tmp/ftptmpfile</clientpath>"\
        "<ptype>1</ptype>"\
        "<srvpath>/A/tmp/tcpsever</srvpath>"\
        "<andchild>true</andchild>"\
        "<matchname>*.xml,*.txt,*.sh</matchname>"\
        "<timetvl>10</timetvl>"\
        "<timeout>50</timeout>"\
        "<pname>tcpputfiles_surfdata</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，采用tcp协议把文件上传给服务端。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("ip            服务端的IP地址。\n");
    printf("port          服务端的端口。\n");
    printf("ptype         文件上传成功后的处理方式：1-删除文件；2-移动到备份目录。\n");
    printf("clientpath    本地文件存放的根目录。\n");
    printf("clientpathbak 文件成功上传后，本地文件备份的根目录，当ptype==2时有效。\n");
    printf("andchild      是否上传clientpath目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
    printf("matchname     待上传文件名的匹配规则，如\"*.TXT,*.XML\"\n");
    printf("srvpath       服务端文件存放的根目录。\n");
    printf("timetvl       扫描本地目录文件的时间间隔，单位：秒，取值在1-30之间。\n");
    printf("timeout       本程序的超时时间，单位：秒，视文件大小和网络带宽而定，建议设置50以上。\n");
    printf("pname         进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");

}

bool activetest()//抖腿一下
{
    sendbuffer="<activetest>ok</activetest>";
    if(tcpclient.write(sendbuffer)==false){
        logfile.write("tcpclient.write(%s) failed.\n",sendbuffer.c_str());
        return false;
    }
    logfile.write("发送:%s succeed\n",sendbuffer.c_str());
    if(tcpclient.read(recebuffer,20)==false){
        logfile.write("tcpclient.read(%s) failed.\n",recebuffer.c_str());
        return false;
    }
    logfile.write("接收:%s succeed\n",recebuffer.c_str());
    return true;
}

bool login(const char *argv)
{
    sformat(sendbuffer,"<clienttype>1</clienttype>%s",argv);
    if(tcpclient.write(sendbuffer)==false){
        logfile.write("login send %s failed\n",sendbuffer.c_str());
        return false;
    }
    
    if(tcpclient.read(recebuffer,20)==false){
        logfile.write("login receive %s failed\n",recebuffer.c_str());
        return false;
    }
    logfile.write("登录成功:%s,%d\n",starg.ip,starg.port);
    return true;
}

bool tcpputfiles()
{
    cdir dir;
    if(dir.opendir(starg.clientpath,starg.matchname,10000,starg.andchild)==false){
        logfile.write("dir.opendir failed.\n");
        return false;
    }
    while(dir.readdir()){
        sformat(sendbuffer,"<filename>%s</filename><mtime>%s</mtime><size>%d</size>",
        dir.m_ffilename.c_str(),dir.m_mtime.c_str(),dir.m_filesize);
        logfile.write("sendbuffer:%s\n",sendbuffer.c_str());
        if(tcpclient.write(sendbuffer)==false){
            logfile.write("tcpclient.write failed:%s",sendbuffer);
            return false;
        }
    //发送文件内容
        logfile.write("send %s ",dir.m_ffilename.c_str());
        if(sendfile(dir.m_ffilename,dir.m_filesize)==false){
            logfile.write("failed.\n");
            return false;
        }
        else{logfile.write("ok.\n");}
    //接收回复报文
        if(tcpclient.read(recebuffer,20)==false){
            logfile.write("tcpclient.read failed.\n");
            return false;
        }
        //处理报文（删除本地文件或保留备份）
        ackmessage(recebuffer);
    }
    return true;
}

bool ackmessage(const string &buffer)
{
    //"<filename>%s</filename><result>ok<result>\n",clientfilename
    string filename;
    string ok;
    getxmlbuffer(buffer,"filename",filename);
    getxmlbuffer(buffer,"ok",ok);
    if(ok!="ok")
        return true;//有逻辑疑问
    if(starg.ptype==1){
        if(remove(filename.c_str())!=0){
            logfile.write("remove (%s) failed.\n",filename);
            return false;
        }
    }
    if(starg.ptype==2){
        string bakfilename=filename;
        replacestr(bakfilename,starg.clientpath,starg.clientpathbak,false);   // 注意，第4个参数一定要填false。
        if (renamefile(filename,bakfilename)==false) 
        { logfile.write("renamefile(%s,%s) failed.\n",filename.c_str(),bakfilename.c_str()); return false; }
    }
    return true;
}

bool sendfile(const string &filename, const int filesize)
{
    int hasreaded=0;//已经读的
    int curread;//这次要读的
    cifile ifile;
    char buff[1000];
    if(ifile.open(filename,ios::binary|ios::in)==false){
        logfile.write("ifile,open failed.\n");
        return false;
    }
    while(true){
        if(filesize-hasreaded>1000) 
            curread=1000;
        else
            curread = filesize-hasreaded;
        ifile.read(buff,curread);
        if(tcpclient.write(buff,curread)==false){
            logfile.write("tcpclient.wriet failed.\n");
            return false;
        }
        hasreaded+=curread;
        if(hasreaded==filesize) break;
    }
    return true;
}

bool xmltoarg(const char* xml)
{
    memset(&starg,0,sizeof(struct st_srg));

    getxmlbuffer(xml,"ip",starg.ip,30);
    if(strlen(starg.ip)==0){
        logfile.write("ip faild\n");
        return false;
    }
    getxmlbuffer(xml,"port",starg.port);
    if(starg.port==0){
        logfile.write("port failed\n");
        return false;
    }
    getxmlbuffer(xml,"clientpath",starg.clientpath,255); 
    if (strlen(starg.clientpath)==0)
    { logfile.write("clientpath falied\n");  return false; }


 // 文件上传成功后本地文件的处理方式：1-删除文件；2-移动到备份目录。
    getxmlbuffer(xml,"ptype",starg.ptype);   
    if ( (starg.ptype!=1) && (starg.ptype!=2))
    { logfile.write("ptype is error.\n"); return false; }

    if (starg.ptype==2)
    {
        getxmlbuffer(xml,"localpathbak",starg.clientpathbak,255); // 上传后客户端文件的备份目录。
        if (strlen(starg.clientpathbak)==0) { logfile.write("clientpathbak is null.\n");  return false; }
    }

    getxmlbuffer(xml,"andchild",starg.andchild);
    if((starg.andchild!=false)&&(starg.andchild!=true)){
        logfile.write("andchild failed.\n");
        return false;
    }
    
    getxmlbuffer(xml,"matchname",starg.matchname,255);
    if(strlen(starg.matchname)==0){
        logfile.write("match failed..\n");
        return false;
    }
    
    getxmlbuffer(xml,"srvpath",starg.srvpath,255);
    if(strlen(starg.srvpath)==0){
        logfile.write("srvpath failed..\n");
        return false;
    }
    getxmlbuffer(xml,"timetvl",starg.timetvl);
    if (starg.timetvl==0) { logfile.write("timetvl is null.\n"); return false; }

    // 扫描本地目录文件的时间间隔（执行上传任务的时间间隔），单位：秒。
    // starg.timetvl没有必要超过30秒。
    if (starg.timetvl>30) starg.timetvl=30;

    // 进程心跳的超时时间，一定要大于starg.timetvl。
    getxmlbuffer(xml,"timeout",starg.timeout);
    if (starg.timeout==0) { logfile.write("timeout is null.\n"); return false; }
    if (starg.timeout<=starg.timetvl)  { logfile.write("starg.timeout(%d) <= starg.timetvl(%d).\n",starg.timeout,starg.timetvl); return false; }

    getxmlbuffer(xml,"pname",starg.pname,50);
    //if (strlen(starg.pname)==0) { logfile.write("pname is null.\n"); return false; }

    return true;
}

void EXIT(int sig)
{
    logfile.write("程序退出,sig=%d\n\n",sig);
    exit(0);
}
