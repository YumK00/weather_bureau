#include"mpublic.h"
#include"ftp.h"
using namespace idc;
cpactive pactive;

void EXIT(int sig);
void help();

struct st_fileinfo              // 文件信息的结构体。
{
    string filename;           // 文件名。
    string mtime;              // 文件时间。
    st_fileinfo()=default;
    st_fileinfo(const string &in_filename,const string &in_mtime):filename(in_filename),mtime(in_mtime) {}
    void clear() { filename.clear(); mtime.clear(); }
}; 

map<string,string> mfromok;             // 容器一：存放已下载成功文件，从starg.okfilename参数指定的文件中加载。
list<struct st_fileinfo> vfromnlist;   // 容器二：下载前列出服务端文件名的容器，从nlist文件中加载。
list<struct st_fileinfo> vtook;        // 容器三：本次不需要下载的文件的容器。
list<struct st_fileinfo> vdownload;    // 容器四：本次需要下载的文件的容器。

bool loadlistdfile();//把匹配的文件名用结构体st_fileinfo存到vformnlist里

bool  downloadfiles();//把容器中的文件名对应的服务器端的文件下载到本地,然后处理源文件

// 加载starg.okfilename文件中的数据到容器vfromok中。
bool loadokfile();

        // 比较vfromnlist和vfromok，得到vtook和vdownload。
bool compmap();

        // 把容器vtook中的数据写入starg.okfilename文件，覆盖之前的旧starg.okfilename文件。
bool writetookfile();

bool appendtookfile(st_fileinfo &s);


cftpclient ftp;
clogfile logfile;

struct st_arg
{
    char host[31];                        // 远程服务端的IP和端口。
    int    mode;                           // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    char username[31];               // 远程服务端ftp的用户名。
    char password[31];                // 远程服务端ftp的密码。
    char remotepath[256];          // 远程服务端存放文件的目录。
    char localpath[256];              // 本地文件存放的目录。
    char matchname[256];          // 待下载文件匹配的规则。
    int   ptype;                            // 下载后服务端文件的处理方式：1-什么也不做；2-删除；3-备份。
    char remotepathbak[256];   // 下载后服务端文件的备份目录。
    char okfilename[256];          // 已下载成功文件信息存放的文件。
    bool checkmtime;                // 是否需要检查服务端文件的时间，true-需要，false-不需要，缺省为false。
    int  timeout;                         // 进程心跳超时的时间。
    char pname[51];                  // 进程名，建议用"ftpgetfiles_后缀"的方式。
} starg;
bool xmltoarg(const char* xmlstr);

int main(int argc,char * argv[]){
    //获取ftp服务器的文件存到某个文件里参数就需要
    // 日志文件名，ftp端口，用户名，密码，主动模式还是被动模式，服务器文件名，本地目录名，匹配规则
    if(argc!=3){
        help();
        return -1;
    }
    signal(2,EXIT);
    signal(15,EXIT);

    if(logfile.open(argv[1])==false){
        cout<<"logfile.open failed:"<<argv[1]<<endl;
        return -1;
    }
     xmltoarg(argv[2]);//拆分xml的信息并存到starg结构体中
     
     pactive.addpinfo(starg.timeout,starg.pname,&logfile);

     //登录
     if(ftp.login(starg.host,starg.username,starg.password,starg.mode)==false){
         logfile.write("ftp.login(%s,%s,%s) failed.\n%s\n",starg.host,starg.username,starg.password,ftp.response());
         return -1;
     }
     //logfile.write("ftp.login(%s,%s,%s) ok.\n",starg.host,starg.username,starg.password);

     //改变工作目录,也可以不改变直接ftp.nlist(starg.remotepath,....)
     //但是这样每个文件会传递绝对路径，传递效率偏低，况且我们知道是哪个目录下的
     if(ftp.chdir(starg.remotepath)==false){
        logfile.write("ftp.chdir(%s) failed.\n",starg.remotepath);
        return false;
     }
     //logfile.write("ftp.chdir(%s) ok.\n",starg.remotepath);

     //列出目录下所有的文件到/A/tmp/nlist/ftpgetfiles_%d.nlist里,加getid防止重复
     if (ftp.nlist(".",sformat("/A/tmp/nlist/ftpgetfiles_%d.nlist",getpid())) == false)
    {
        logfile.write("ftp.nlist(%s) failed.\n%s\n",starg.remotepath,ftp.response()); return -1;
    }
    //logfile.write("ftp.nlist(%s) ok.\n",sformat("/tmp/nlist/ftpgetfiles_%d.nlist",getpid()).c_str());

    pactive.uptatime();
    
    //把文件目录存到结构体中
    if(loadlistdfile()==false){
        logfile.write("loadlistfile failed");
        return -1;
    }
    downloadfiles();//下载到本地  //处理服务端的源文件 ptype下载后服务端文件的处理方式：1-什么也不做；2-删除；3-备份。
    pactive.uptatime();
    ftp.logout();
    return 0;
}

void help(){
    printf("Sample:/A/tools/bin/procctl 30 /A/tools/bin/ftpgetfiles /A/log/ftpgetfiles/ftpgetfiles_surfdata.log " \
             "\"<host>192.168.5.5:21</host><mode>1</mode>"\
             "<username>dba</username><password>222222</password>"\
             "<remotepath>/home/dba/tmp</remotepath>"\
             "<localpath>/A/tmp/ftptmpfile</localpath>"\
             "<matchname>*.csv</matchname>"\
             "<checkmtime>true</checkmtime>"\
             "<ptype>1</ptype>"\
             "<okfilename>/A/tmp/ftplist/ftpgetfiles_test.xml</okfilename>"\
             "<timeout>30</timeout>"\
             "<pname>ftpgetfiles_test</pname>"\
             "<remotepathbak>/tmp/dbadata</remotepathbak>\"\n\n");
}

bool loadlistdfile()
{
    vfromnlist.clear();
    cifile ifile;
    if(ifile.open(sformat("/A/tmp/nlist/ftpgetfiles_%d.nlist",getpid()))==false){
        logfile.write("loadlistfile open /A/tmp/nlist/ftpgetfiles.nlist failed");
        return false;
    }
    string str;
    while(true){
        if(ifile.readline(str)==false) break;
        if(matchstr(str,starg.matchname)==false) continue;
        //获取文件的时间,因为服务器上的文件不能在线，所以该事件就是最后修改时间
        if(starg.checkmtime==true&&starg.ptype==1){
            if(ftp.mtime(str)==false){
                logfile.write("ftp.mtime(%s) failed.\n",str.c_str());
                return false;
            }
        }
        //emplace_back会用这些参数调用结构体的构造函数，等价于先创建一个结构体再pushback进容器,但是没有结构体开销
        vfromnlist.emplace_back(str,ftp.m_mtime);
    }

    //  for (auto &aa:vfromnlist)
    //    logfile.write("loadlistedfile   filename=%s,mtime=%s\n",aa.filename.c_str(),aa.mtime.c_str());

    ifile.closeandremove();
    
    return true;
}

bool downloadfiles()
{
    
    if(starg.ptype==1){
        // 加载starg.okfilename文件中的数据到容器vfromok中。
        loadokfile();

        // 比较vfromnlist和vfromok，得到vtook和vdownload。
        compmap();

        // 把容器vtook中的数据写入starg.okfilename文件，覆盖之前的旧starg.okfilename文件。
        writetookfile();
    }
    if(starg.ptype==2||starg.ptype==3){
        vfromnlist.swap(vdownload);
    }
    string remotename,localename;
    for(auto &aa:vdownload){
        remotename = sformat("%s/%s", starg.remotepath, aa.filename.c_str());
        localename = sformat("%s/%s", starg.localpath, aa.filename.c_str());
        logfile.write("get %s ...",remotename.c_str());
        if(ftp.get(remotename,localename)==false){
            logfile<<" failed,response = %s\n",ftp.response();
            return false;
        }
        logfile<<" ok.\n";

        //处理原文件，1-什么也不做，但是只下载新增的和修改的文件；2-删除；3-备份。
        if(starg.ptype==1){
            appendtookfile(aa);
        }
        if(starg.ptype==2){
            if(ftp.ftpdelete(remotename)==false){
                logfile.write("ftp.ftpdelete failed.\t%s",ftp.response());
                return false;
            }
        }
        if(starg.ptype==3){
            string filebakname = sformat("%s/%s",starg.remotepathbak,aa.filename.c_str());
            if(ftp.ftprename(remotename,filebakname)==false){
                logfile.write("ftp.ftprename failed.\t%s",ftp.response());
                return false;
            }
        }

    }
    return true;
}

bool loadokfile()
{
    cifile ifile;
    if(ifile.open(starg.okfilename)==false)   return true;//第一次运行没有这个文件返回true
    string info;
    st_fileinfo fileinfo;
    while(true){
        fileinfo.clear();
        if(ifile.readline(info)==false) break;
        getxmlbuffer(info,"filename",fileinfo.filename);
        getxmlbuffer(info,"mtime",fileinfo.mtime);
        mfromok[fileinfo.filename]=fileinfo.mtime;
    }

    //logfile.write("mfrmok\n");
    //for (auto &aa:mfromok)
    //   logfile.write("filename=%s,mtime=%s\n",aa.first.c_str(),aa.second.c_str());
    return true;
}

bool compmap()
{
    vtook.clear();
    vdownload.clear();
    string str;
    for(auto &aa:vfromnlist){
        auto it = mfromok.find(aa.filename);
        if(it!=mfromok.end()){
            if(starg.checkmtime==true){
                if(aa.mtime==it->second){
                    vtook.push_back(aa);
                }
                else{
                    vdownload.push_back(aa);
                }
            }
            else{
                vtook.push_back(aa);
            }
        }
        else{
            vdownload.push_back(aa);
        }
    }
    return true;
}

bool writetookfile()
{
    cofile ofile;
    if(ofile.open(starg.okfilename)==false){
        logfile.write("file.open(%s) failed.\n",starg.okfilename);
        return false;
    }
    for(auto &aa:vtook){
        ofile.writeline("<filename>%s</filename><mtime>%s</mtime>\n",aa.filename.c_str(),aa.mtime.c_str());
    }
    return true;
}

bool appendtookfile(st_fileinfo &stfileinfo)
{
    cofile ofile;
    if(ofile.open(starg.okfilename,false,ios::out|ios::app)==false){
        logfile.write("appendtookfile ofile.open(okfilename) failed.\n");
        return false;
    }
    ofile.writeline("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo.filename.c_str(),stfileinfo.mtime.c_str());
    return true;
}

bool xmltoarg(const char *xmlstr)
{
    memset(&starg,0,sizeof(struct st_arg));
    getxmlbuffer(xmlstr,"host",starg.host,30);
    if(strlen(starg.host)==0){
        logfile.write("host getxmlbuffer failed");
        return false;
    }
    getxmlbuffer(xmlstr,"mode",starg.mode);
    if(starg.mode!=2)
        starg.mode=1;
    getxmlbuffer(xmlstr,"username",starg.username);
    if(strlen(starg.username)==0){
        logfile.write("username getxmlbuffer failed");
        return false;
    }
    getxmlbuffer(xmlstr,"password",starg.password);
    if(strlen(starg.password)==0){
        logfile.write("passwd getxmlbuffer failed");
        return false;
    }
    getxmlbuffer(xmlstr,"remotepath",starg.remotepath);
    if(strlen(starg.remotepath)==0){
        logfile.write("remotepath getxmlbuffer failed");
        return false;
    }
    getxmlbuffer(xmlstr,"localpath",starg.localpath);
    if(strlen(starg.localpath)==0){
        logfile.write("localpath getxmlbuffer failed");
        return false;
    }
    getxmlbuffer(xmlstr,"matchname",starg.matchname);
    if(strlen(starg.matchname)==0){
        logfile.write("matchname getxmlbuffer failed");
        return false;
    }
    getxmlbuffer(xmlstr,"ptype",starg.ptype);
    if(starg.ptype!=1&&starg.ptype!=2&&starg.ptype!=3){
        logfile.write("ptype getxmlbuffer failed");
        return false;
    }    
    if(starg.ptype==3){
        getxmlbuffer(xmlstr,"remotepathbak",starg.remotepathbak);
        if(strlen(starg.remotepathbak)==0){
            logfile.write("remotepathbak getxmlbuffer failed");
            return false;
        }
    }
    if (starg.ptype==1) 
    {
        getxmlbuffer(xmlstr,"okfilename",starg.okfilename,255); // 已下载成功文件名清单。
        if ( strlen(starg.okfilename)==0 ) { 
            logfile.write("okfilename is null.\n");
            return false;
        }
        // 是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。
        getxmlbuffer(xmlstr,"checkmtime",starg.checkmtime);
    }
    getxmlbuffer(xmlstr,"timeout",starg.timeout);   // 进程心跳的超时时间。
    if (starg.timeout==0) { logfile.write("timeout is null.\n");  return false; }

    getxmlbuffer(xmlstr,"pname",starg.pname,50);     // 进程名。

    return true;
}

void EXIT(int sig){
    cout<<"sig = "<<sig;
    exit(0);
}
