#include"mpublic.h"
using namespace idc;
void EXIT(int flog);

cpactive pactive;

int main(int argc,char*argv[]){
    if (argc != 4)
    {
        printf("\n");
        printf("Using:/project/tools/bin/gzipfiles pathname matchstr timeout\n\n");

        printf("Example:/A/tools/bin/gzipfiles /A/idc/observedata \"*.xml,*.json\" 0.01\n");
        cout << R"(        /project/tools/bin/gzipfiles /log/idc "*.log.20*" 0.02)" << endl;
        printf("        /project/tools/bin/procctl 300 /project/tools/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools/bin/procctl 300 /project/tools/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

        printf("这是一个工具程序，用于压缩历史的数据文件或日志文件。\n");
        printf("本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部压缩，timeout可以是小数。\n");
        printf("本程序不写日志文件，也不会在控制台输出任何信息。\n");
        printf("本程序调用/usr/bin/gzip命令压缩文件。\n\n\n");

        return -1;
	}
    pactive.addpinfo(30,"gzipfiles");
    
    closeioandsignal();
    signal(2,EXIT);
    signal(15,EXIT);

    string strtimeout =ltime1("yyyymmddhh24miss",0-(int)(atof(argv[3])*24*60*60));
    cout<<"strtimeout="<<strtimeout<<endl;
    cdir dir;
    if(dir.opendir(argv[1],argv[2],10000,true)==false){
       cout<<"open failed:"<<argv[1]<<endl;
       return -1;
    }

    while(dir.readdir()==true){
        //cout<<dir.m_ffilename<<":"<<dir.m_mtime<<endl;
        if((dir.m_mtime<strtimeout)&&(matchstr(dir.m_filename,"*.zp"))==false){
            string strcmd = "/usr/bin/gzip -f " + dir.m_ffilename + " 1>/dev/null 2>/dev/null";
            if(system(strcmd.c_str())==0)
                cout<<"gzip success:"<<dir.m_ffilename<<endl;
            
            else
                cout<<"gzip failed"<<dir.m_ffilename<<endl;
        } 
    }
    return 0;
}

void EXIT(int flog){
    cout<<"程序退出,sig = "<<flog<<endl;
    exit(0);
}