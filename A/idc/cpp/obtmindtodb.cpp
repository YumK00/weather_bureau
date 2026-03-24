// //#include "_public.h"  // 开发框架的头文件。
// //#include "_ooci.h"    // 操作Oracle的头文件。
 #include "idcapp.h"    
 using namespace idc;

// clogfile logfile;        // 日志文件。
// connection conn;    // 数据库连接。
// cpactive pactive;     // 进程的心跳。

// // 业务处理主函数。
// bool _obtmindtodb(const char *pathname,const char *connstr,const char *charset);

// void EXIT(int sig);     // 程序退出的信号处理函数。

// int main(int argc,char *argv[])
// {
//     // 帮助文档。
//     if (argc!=5)
//     {
//         printf("\n");
//         printf("Using:./obtmindtodb pathname connstr charset logfile\n");

//         printf("Example:/A/tools/bin/procctl 10 /A/idc/bin/obtmindtodb /A/idc/observedata "\
//                   "\"idc/idcpwd@snorcl11g_5\" \"Simplified Chinese_China.AL32UTF8\" /A/log/idc/obtmindtodb.log\n\n");

//         printf("本程序用于把全国气象观测数据文件入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式，数据只插入，不更新。\n");
//         printf("pathname 全国气象观测数据文件存放的目录。\n");
//         printf("connstr  数据库连接参数：username/password@tnsname\n");
//         printf("charset  数据库的字符集。\n");
//         printf("logfile  本程序运行的日志文件名。\n");
//         printf("程序每10秒运行一次，由procctl调度。\n\n\n");

//         return -1;
//     }

//     // 关闭全部的信号和输入输出。
//     // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
//     // 但请不要用 "kill -9 +进程号" 强行终止。
//     // clostioandsignal(true); 
//     signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

//     // 打开日志文件。
//     if (logfile.open(argv[4])==false)
//     {
//         printf("打开日志文件失败（%s）。\n",argv[4]); return -1;
//     }

//     pactive.addpinfo(30,"obtmindtodb");   // 进程心跳。

//     // 业务处理主函数。
//     _obtmindtodb(argv[1],argv[2],argv[3]);

//     return 0;
// }

// void EXIT(int sig)
// {
//     logfile.write("程序退出，sig=%d\n\n",sig);

//     // 可以不写，在析构函数中会回滚事务和断开与数据库的连接。
//     conn.rollback();
//     conn.disconnect();   

//     exit(0);
// }

// // 业务处理主函数。
// bool _obtmindtodb(const char *pathname,const char *connstr,const char *charset)
// {
//     // 1）打开存放气象观测数据文件的目录。
//     cdir dir;
//     if (dir.opendir(pathname,"*.xml,*.csv")==false)
//     {
//         logfile.write("dir.opendir(%s) failed.\n",pathname); return false;
//     }

//     CZHOBTMIND ZHOBTMIND(conn,logfile);  // 操作气象观测数据表的对象。

//     // 2）用循环读取目录中的每个文件。
//     while (true)
//     {
//         // 读取一个气象观测数据文件（只处理*.xml和*.csv）。
//         if (dir.readdir()==false) break;

//         // 如果有文件需要处理，判断与数据库的连接状态，如果是未连接，就连上数据库。
//         if (conn.isopen()==false)
//         {
//             if (conn.connecttodb(connstr,charset)!=0)
//             {
//                 logfile.write("connect database(%s) failed.\n%s\n",connstr,conn.message()); return false;
//             }
    
//             logfile.write("connect database(%s) ok.\n",connstr);
//         }

//         // 打开文件。
//         cifile ifile;
//         if (ifile.open(dir.m_ffilename)==false)
//         {
//             logfile.write("file.open(%s) failed.\n",dir.m_ffilename.c_str()); return false;
//         }

//         int  totalcount=0;     // 文件的总记录数。
//         int  insertcount=0;   // 成功插入记录数。
//         ctimer timer;            // 计时器，记录每个数据文件的处理耗时。
//         bool bisxml=matchstr(dir.m_ffilename,"*.xml");  // 文件格式，true-xml；false-csv。

//         string strbuffer;      // 存放从文件中读取的一行数据。

//         // 如果是csv文件，扔掉第一行。
//         if (bisxml==false)  ifile.readline(strbuffer);

//         // 读取文件中的每一行，插入到数据库的表中。
//         while(true)
//         {
//             // 从文件中读取一行。
//             if (bisxml==true)
//             {
//                 if (ifile.readline(strbuffer,"<endl/>")==false) break;     // xml文件的行结束标志是<endl/>
//             }
//             else
//             {
//                 if (ifile.readline(strbuffer)==false) break;                      // csv文件没有行结束标志。 
//             }

//             totalcount++;       // 文件的总记录数加1。

//             // 解析行的内容（*.xml和*.csv的方法不同），把数据存放在结构体中。
//             ZHOBTMIND.splitbuffer(strbuffer,bisxml);

//             // 把解析后的数据入库（插入到数据库的表中）。
//             if (ZHOBTMIND.inserttable()==true) insertcount++;       // 成功插入的记录数加1。
//         }

//         // 关闭并删除已处理的文件，提交事务。
//         ifile.closeandremove();
//         conn.commit();
//         logfile.write("已处理文件%s（totalcount=%d,insertcount=%d），耗时%.2f秒。\n",\
//                               dir.m_ffilename.c_str(),totalcount,insertcount,timer.elapsed());
//         pactive.uptatime();   // 进程心跳。
//     }

//     return true;
// }



clogfile logfile;
connection conn;
struct st_zhobtmind 
{
    char obtid[6];            // 站点代码。
    char ddatetime[15];       // 数据时间，精确到分钟。
    char t[11];               // 温度，单位：0.1摄氏度。
    char p[11];               // 气压，单位：0.1百帕。
    char u[11];               // 相对湿度，0-100之间的值。
    char wd[11];              // 风向，0-360之间的值。
    char wf[11];              // 风速：单位0.1m/s。
    char r[11];               // 降雨量：0.1mm。
    char vis[11];             // 能见度：0.1米。
} zhobtmind;

void EXIT(int sig);

int main(int argc,char *argv[]){
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtmindtodb pathname connstr charset logfile\n");
        printf("Example:/A/tools/bin/procctl 10 /A/idc/bin/obtmindtodb /A/idc/observedata "\
                  "\"idc/idcpwd@snorcl11g_5\" \"Simplified Chinese_China.AL32UTF8\" /A/log/idc/obtmindtodb1.log\n\n");
        printf("本程序用于把全国气象观测数据文件入库到T_ZHOBTMIND表中，支持xml文件格式，数据只插入，不更新。\n");
        printf("pathname 全国气象观测数据文件存放的目录。\n");
        printf("connstr  数据库连接参数：username/password@tnsname\n");
        printf("charset  数据库的字符集。\n");
        printf("logfile  本程序运行的日志文件名。\n");
        printf("程序每10秒运行一次，由procctl调度。\n\n\n");
        return -1;
    }

    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    if (logfile.open(argv[4])==false){
        printf("打开日志文件失败(%s).\n",argv[4]); 
        return -1;
    }

    // 先连接数据库
    if (conn.connecttodb(argv[2],argv[3])!=0)
    {
        logfile.write("connect database(%s) failed.\n%s\n",argv[2],conn.message()); 
        return -1;
    }
    logfile.write("connect database(%s) ok.\n",argv[2]);

    cdir dir;
    if(dir.opendir(argv[1],"*.xml",10000,false,false)==false){
        logfile.write("dir.opendir %s  failed",argv[1]);
        return -1;
    }

    while(true){
        if(dir.readdir()==false) break;

        // 检查连接状态，如果断开则重连
        if (conn.isopen()==false)
        {
            logfile.write("数据库连接断开，重新连接...\n");
            if (conn.connecttodb(argv[2],argv[3])!=0)
            {
                logfile.write("重新连接数据库(%s) failed.\n%s\n",argv[2],conn.message()); 
                sleep(10);
                continue;
            }
            logfile.write("重新连接数据库(%s) ok.\n",argv[2]);
        }

        cifile ifile;
        if(ifile.open(dir.m_ffilename)==false){
            logfile.write("ifile.open %s failed.\n",dir.m_ffilename.c_str());
            continue;
        }
        
        string buffer;
        ifile.readline(buffer);
        
        // 为每个文件创建新的stmt对象
        sqlstatement stmt(&conn);
        if (stmt.prepare("insert into ZHOBTMIND(obtid,datetime,t,p,u,wd,wf,r,vis,keyid) "
                         "values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,SEQ_ZHOBTMIND.nextval)") != 0) {
            logfile.write("stmt.prepare failed: %s\n", stmt.message());
            ifile.close();
            continue;
        }
        
        stmt.bindin(1,zhobtmind.obtid,5);
        stmt.bindin(2,zhobtmind.ddatetime,14);
        stmt.bindin(3, zhobtmind.t, 10);
        stmt.bindin(4, zhobtmind.p, 10);
        stmt.bindin(5, zhobtmind.u, 10);
        stmt.bindin(6, zhobtmind.wd, 10);
        stmt.bindin(7, zhobtmind.wf, 10);
        stmt.bindin(8, zhobtmind.r, 10);
        stmt.bindin(9, zhobtmind.vis, 10);
        
        logfile.write("开始处理文件：%s\n", dir.m_ffilename.c_str());
        
        int inserted = 0, failed = 0;
        while(true){
            if(ifile.readline(buffer,"<endl/>")==false) break;
            
            // 初始化结构体
            memset(&zhobtmind, 0, sizeof(zhobtmind));
            
            // 解析XML数据
            getxmlbuffer(buffer,"obtid",zhobtmind.obtid,5);
            getxmlbuffer(buffer,"ddatetime",zhobtmind.ddatetime,14);
            
            // 记录原始数据以便调试
            logfile.write("原始数据 - obtid:%s, ddatetime:%s\n", zhobtmind.obtid, zhobtmind.ddatetime);
            
            char tmp[11];
            getxmlbuffer(buffer,"t",tmp,10);     
            if (strlen(tmp)>0) snprintf(zhobtmind.t,10,"%d",(int)(atof(tmp)*10));
            
            getxmlbuffer(buffer,"p",tmp,10);    
            if (strlen(tmp)>0) snprintf(zhobtmind.p,10,"%d",(int)(atof(tmp)*10));
            
            getxmlbuffer(buffer,"u",zhobtmind.u,10);
            getxmlbuffer(buffer,"wd",zhobtmind.wd,10);
            
            getxmlbuffer(buffer,"wf",tmp,10);  
            if (strlen(tmp)>0) snprintf(zhobtmind.wf,10,"%d",(int)(atof(tmp)*10));
            
            getxmlbuffer(buffer,"r",tmp,10);     
            if (strlen(tmp)>0) snprintf(zhobtmind.r,10,"%d",(int)(atof(tmp)*10));
            
            getxmlbuffer(buffer,"vis",tmp,10);  
            if (strlen(tmp)>0) snprintf(zhobtmind.vis,10,"%d",(int)(atof(tmp)*10));
            
            // 检查必要字段
            if (strlen(zhobtmind.obtid) == 0 || strlen(zhobtmind.ddatetime) == 0) {
                logfile.write("跳过无效记录：obtid或ddatetime为空\n");
                failed++;
                continue;
            }
            
            // 执行插入
            if(stmt.execute()!=0){
                failed++;
                logfile.write("插入失败 - obtid:%s, ddatetime:%s\n", zhobtmind.obtid, zhobtmind.ddatetime);
                logfile.write("错误信息：%s\n", stmt.message());
                // 如果是主键冲突（重复插入），可能是正常情况
                if (stmt.rc() == 1) {
                    logfile.write("主键冲突，可能是重复数据\n");
                }
            } else {
                inserted++;
            }
        }
        
        ifile.close();
        
        if (inserted > 0) {
            conn.commit();
            logfile.write("文件%s处理完成：成功插入%d条，失败%d条\n", 
                          dir.m_ffilename.c_str(), inserted, failed);
        } else {
            conn.rollback();
            logfile.write("文件%s没有数据插入\n", dir.m_ffilename.c_str());
        }
        
        // 可选：处理完后移动或删除文件
        // string newfilename = dir.m_ffilename + ".bak";
        // rename(dir.m_ffilename.c_str(), newfilename.c_str());
    }

    return 0;
}

/*
clogfile logfile;
connection conn;
struct st_zhobtmind 
    {
        char obtid[6];            // 站点代码。
        char ddatetime[15];  // 数据时间，精确到分钟。
        char t[11];                 // 温度，单位：0.1摄氏度。
        char p[11];                // 气压，单位：0.1百帕。
        char u[11];                // 相对湿度，0-100之间的值。
        char wd[11];             // 风向，0-360之间的值。
        char wf[11];              // 风速：单位0.1m/s。
        char r[11];                // 降雨量：0.1mm。
        char vis[11];             // 能见度：0.1米。
    }zhobtmind;

void EXIT(int sig);

int main(int argc,char *argv[]){
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtmindtodb pathname connstr charset logfile\n");

        printf("Example:/A/tools/bin/procctl 10 /A/idc/bin/obtmindtodb /A/idc/observedata "\
                  "\"idc/idcpwd@snorcl11g_5\" \"Simplified Chinese_China.AL32UTF8\" /A/log/idc/obtmindtodb1.log\n\n");

        printf("本程序用于把全国气象观测数据文件入库到T_ZHOBTMIND表中，支持xml文件格式，数据只插入，不更新。\n");
        printf("pathname 全国气象观测数据文件存放的目录。\n");
        printf("connstr  数据库连接参数：username/password@tnsname\n");
        printf("charset  数据库的字符集。\n");
        printf("logfile  本程序运行的日志文件名。\n");
        printf("程序每10秒运行一次，由procctl调度。\n\n\n");

        return -1;
    }

    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    if (logfile.open(argv[4])==false){
        printf("打开日志文件失败(%s).\n",argv[4]); 
        return -1;
    }


    cdir dir;
    if(dir.opendir(argv[1],"*.xml",10000,false,false)==false){
        logfile.write("dir.opendir %s  failed",argv[1]);
        return -1;
    }

    sqlstatement stmt(&conn);
    stmt.prepare("insert into ZHOBTMIND(obtid,datetime,t,p,u,wd,wf,r,vis,keyid)\
    values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,SEQ_ZHOBTMIND.nextval)");
    stmt.bindin(1,zhobtmind.obtid,5);
    stmt.bindin(2,zhobtmind.ddatetime,14);
    stmt.bindin(3, zhobtmind.t, 10);         // 温度长度10
    stmt.bindin(4, zhobtmind.p, 10);         // 气压长度10
    stmt.bindin(5, zhobtmind.u, 10);         // 湿度长度10
    stmt.bindin(6, zhobtmind.wd, 10);        // 风向长度10
    stmt.bindin(7, zhobtmind.wf, 10);        // 风速长度10
    stmt.bindin(8, zhobtmind.r, 10);         // 降雨量长度10
    stmt.bindin(9, zhobtmind.vis, 10);       // 能见度长度10
    
    while(true){
        if(dir.readdir()==false)     break;

        if (conn.isopen()==false)
        {
            if (conn.connecttodb(argv[2],argv[3])!=0)
            {
                logfile.write("connect database(%s) failed.\n%s\n",argv[2],conn.message()); 
                return -1;
            }
    
            logfile.write("connect database(%s) ok.\n",argv[2]);
        }

        cifile ifile;
        if(ifile.open(dir.m_ffilename)==false){
            logfile.write("ifile.open %s failed.\n",dir.m_ffilename.c_str());
            EXIT(-1);
        }
        string buffer;
        ifile.readline(buffer);

        while(true){
            if(ifile.readline(buffer,"<endl/>")==false) break;
            getxmlbuffer(buffer,"obtid",zhobtmind.obtid,5);
            getxmlbuffer(buffer,"ddatetime",zhobtmind.ddatetime,14);
            char tmp[11];
            getxmlbuffer(buffer,"t",tmp,10);     if (strlen(tmp)>0) snprintf(zhobtmind.t,10,"%d",(int)(atof(tmp)*10));
            getxmlbuffer(buffer,"p",tmp,10);    if (strlen(tmp)>0) snprintf(zhobtmind.p,10,"%d",(int)(atof(tmp)*10));
            getxmlbuffer(buffer,"u",zhobtmind.u,10);
            getxmlbuffer(buffer,"wd",zhobtmind.wd,10);
            getxmlbuffer(buffer,"wf",tmp,10);  if (strlen(tmp)>0) snprintf(zhobtmind.wf,10,"%d",(int)(atof(tmp)*10));
            getxmlbuffer(buffer,"r",tmp,10);     if (strlen(tmp)>0) snprintf(zhobtmind.r,10,"%d",(int)(atof(tmp)*10));
            getxmlbuffer(buffer,"vis",tmp,10);  if (strlen(tmp)>0) snprintf(zhobtmind.vis,10,"%d",(int)(atof(tmp)*10));
            if(stmt.execute()!=0){
                if(stmt.rc()!=1){
                    logfile.write("buffer =  %s.\n",buffer.c_str());
                    logfile.write("stmt.execute failed.%s ,%s\n",stmt.sql(),stmt.message());
                }
            }
        }
        ifile.close();
        conn.commit();
    }

    return 0;
}
*/
void EXIT(int sig)
{
    logfile.write("程序退出,sig=%d\n\n",sig);
    conn.rollback();
    conn.disconnect();   
    exit(0);
}

