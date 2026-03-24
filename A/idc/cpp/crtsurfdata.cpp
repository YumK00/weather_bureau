#include"mpublic.h"
using namespace idc;
using namespace std;

clogfile logfile;  

cpactive pactive;

void EXIT(int sign);


// 省   站号  站名 纬度   经度  海拔高度
// 安徽,58015,砀山,34.27,116.2,44.2
struct st_stcode{
    char provname[31];//省份
    char obtid[11];//站点id
    char obtname[31];//站点名称
    double lat;//纬度
    double lon;//经度
    double height;//海拔
};

std::list<struct st_stcode>stlist;//存放信息的容器

bool loadstcode(const string& inifile);//把ini信息存到容器中

//气象站观测数据结构体
struct st_surfdate{
    char obtid[11];          // 站点代码。
    char ddatetime[15];  // 数据时间：格式yyyymmddhh24miss，精确到分钟，秒固定填00。
    int  t;                         // 气温：单位，0.1摄氏度。
    int  p;                        // 气压：0.1百帕。
    int  u;                        // 相对湿度，0-100之间的值。
    int  wd;                     // 风向，0-360之间的值。
    int  wf;                      // 风速：单位0.1m/s
    int  r;                        // 降雨量：0.1mm。
    int  vis;                     // 能见度：0.1米。
};
std::list<st_surfdate>datalist;//存放站点数据结构体

void crtsurfdata();//把站点数据存到datalist结构体内
char strddatetime[15];//因为很多地方要用，所以全局

bool crtsurffile(char* path,const string fmt);

int main(int argc,char* argv[] ){
    if(argc!=5){
        // 如果参数非法，给出帮助文档。
        cout << "Using:procctl./crtsurfdata inifile datafmt outpath logfile \n";//
        cout << "Examples:/A/tools/bin/procctl 600 /A/idc/bin/crtsurfdata /A/idc/ini/stcode.ini /A/idc/observedata /A/log/idc/crtsurfdata.log csv,xml,json\n";

        cout<<"调度程序，时间间隔，此程序，数据地址，生成数据文件夹，生成数据文件名称，文件格式";
        cout << "本程序用于生成气象站点观测的分钟数据，程序十分钟运行一次，由调度模块启动。\n";
        cout << "inifile  气象站点参数文件名。\n";
        cout << "outpath  气象站点数据文件存放的目录。\n";
        cout << "logfile  本程序运行的日志文件名。\n";
        cout << "datafmt  输出数据文件的格式,支持csv、xml和json,中间用逗号分隔。\n\n";
        return -1;
    }
    //closeioandsignal(true);//关闭所有标准输入输出 只让日志文件运行
    //signal(SIGINT,EXIT);//2 ctrl+c
    //signal(SIGTERM,EXIT);//15 也是终止

    pactive.addpinfo(10,"crtsurfdata");

    if(logfile.open(argv[3],ios::app,false,false)==false){
        cout<<"logfile.open"<<argv[3]<<"failed\n";
        return -1;
    }
    
    logfile.write("crtsurfdata开始运行\n");
 
    // 处理业务。
    // 1）从站点参数文件中加载站点参数，存放于stlist容器中；
    //logfile.write(argv[1]);
    if ( loadstcode(argv[1]) ==  false) {
         logfile.write("loadstcode(argv[1]) ==  false");
         EXIT(-1);   
     }
    // 获取观测数据的时间。
    memset(strddatetime,0,sizeof(strddatetime));
    ltime(strddatetime,"yyyymmddhh24miss");   // 获取系统当前时间。
    strncpy(strddatetime+12,"00",2);                   // 把数据时间中的秒固定填00。

    // // 2）根据stlist容器中的站点参数，生成站点观测数据（随机数），生成后的数据存放于容器中；
    crtsurfdata();

    // // 3）把容器datalist中的气象观测数据写入文件，outpath-数据文件存放的目录；datafmt-数据文件的格式，取值：csv、xml和json。
    if (strstr(argv[4],"csv")!=0)    crtsurffile(argv[2],"csv");
    if (strstr(argv[4],"xml")!=0)   crtsurffile(argv[2],"xml");
    if (strstr(argv[4],"json")!=0)  crtsurffile(argv[2],"json");
    logfile.write("crtsurfdata结束运行\n");       
    return 0;
}
 
void EXIT(int sign)
{
    logfile.write("程序退出,signal=%d\n",sign);
    exit(0);
}

bool loadstcode(const string &inifile)
{
    cifile ifile;
    if(ifile.open(inifile)==false){
        logfile.write("infile.open %s error\n",inifile.c_str());
        return false;
    }
    string buf;
    st_stcode stcode;
    ccmdstr cmdstr;
    ifile.readline(buf);
    while(ifile.readline(buf)){
        //logfile.write("strbuffer=%s\n",buf.c_str());
        cmdstr.splittocmd(buf,",",true);
        memset(&stcode,0,sizeof(st_stcode));
        cmdstr.getvalue(0,stcode.provname,30);   // 省
        cmdstr.getvalue(1,stcode.obtid,10);           // 站点代码
        cmdstr.getvalue(2,stcode.obtname,30);     // 站名
        cmdstr.getvalue(3,stcode.lat);                    // 纬度
        cmdstr.getvalue(4,stcode.lon);                   // 经度
        cmdstr.getvalue(5,stcode.height);              // 海拔高度
        stlist.push_back(stcode);
    }
    // for (auto &aa : stlist)
    // {  
    //     cout<<aa.provname<<aa.obtid<<aa.obtname<<"\r"<<aa.lat<<"\r"<<aa.lon<<"\r"<<aa.height<<endl;
    // }
    return true; 
}

// char obtid[11];          // 站点代码。
//     char ddatetime[15];  // 数据时间：格式yyyymmddhh24miss，精确到分钟，秒固定填00。
//     int  t;                         // 气温：单位，0.1摄氏度。
//     int  p;                        // 气压：0.1百帕。
//     int  u;                        // 相对湿度，0-100之间的值。
//     int  wd;                         // 风向，0-360之间的值。
//     int  wf;                      // 风速：单位0.1m/s
//     int  r;                        // 降雨量：0.1mm。
//     int  vis;                    //能见度
void crtsurfdata()
{
    st_surfdate stsurfdata;
    srand(time(0));
    for(auto &ii:stlist){
        memset(&stsurfdata,0,sizeof(stsurfdata));
        strcpy(stsurfdata.obtid,ii.obtid);
        strcpy(stsurfdata.ddatetime,strddatetime);
        stsurfdata.t=rand()%350;
        stsurfdata.p=rand()%265+10000;                       
        stsurfdata.u=rand()%101;                                    
        stsurfdata.wd=rand()%360;                                 
        stsurfdata.wf=rand()%150;                                 
        stsurfdata.r=rand()%16;                                  
        stsurfdata.vis=rand()%5001+100000;
        datalist.push_back(stsurfdata);
    }
    // for (auto &aa : datalist)
    // {
    //     logfile.write("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n", \
    //                          aa.obtid,aa.ddatetime,aa.t/10.0,aa.p/10.0,aa.u,aa.wd,aa.wf/10.0,aa.r/10.0,aa.vis/10.0);
    // }
        
}

bool crtsurffile(char *path, const string fmt)
{
    string filename = string(path)+"/"+"observe_data"+strddatetime+"."+fmt;
    cofile fout;
    if(fout.open(filename,false,ios::out|ios::trunc,false)==false){
        logfile.write("ofile.open(%s) failed.\n",filename.c_str());
        return false; 
    }

    if(fmt=="csv")  fout.writeline("站点代码,数据时间,气温,气压,相对湿度,风向,风速,降雨量,能见度\n");
    if(fmt=="xml") fout.writeline("<data>\n");
    if (fmt=="json")  fout.writeline("{\"data\":[\n");
    for(auto &aa:datalist){
        if(fmt=="csv"){
            fout.writeline("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n",\
                                    aa.obtid,aa.ddatetime,aa.t/10.0,aa.p/10.0,aa.u,aa.wd,aa.wf/10.0,aa.r/10.0,aa.vis/10.0);
        }
        if(fmt=="xml"){
            fout.writeline("<obtid>%s</obtid><ddatetime>%s</ddatetime><t>%.1f</t><p>%.1f</p><u>%d</u>"\
                                   "<wd>%d</wd><wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/>\n",\
                                    aa.obtid,aa.ddatetime,aa.t/10.0,aa.p/10.0,aa.u,aa.wd,aa.wf/10.0,aa.r/10.0,aa.vis/10.0);
        }
        if (fmt=="json") 
        {
            fout.writeline("{\"obtid\":\"%s\",\"ddatetime\":\"%s\",\"t\":\"%.1f\",\"p\":\"%.1f\","\
                                  "\"u\":\"%d\",\"wd\":\"%d\",\"wf\":\"%.1f\",\"r\":\"%.1f\",\"vis\":\"%.1f\"}",\
                                   aa.obtid,aa.ddatetime,aa.t/10.0,aa.p/10.0,aa.u,aa.wd,aa.wf/10.0,aa.r/10.0,aa.vis/10.0);
            // 注意，json文件的最后一条记录不需要逗号，用以下代码特殊处理。
            static int ii=0;     // 已写入数据行数的计数器。
            if (ii<datalist.size()-1)
            {   // 如果不是最后一行。
                fout.writeline(",\n");  ii++;
            }
            else
                fout.writeline("\n");  
        }
    }

    if(fmt=="xml")  fout.writeline("</data>");
    if (fmt == "json")  fout.writeline("]}\n");
    fout.closeandrename();
    logfile.write("生成数据文件%s成功,数据时间%s,记录数%d。\n",filename.c_str(),strddatetime,datalist.size());
    return true;
} 
  