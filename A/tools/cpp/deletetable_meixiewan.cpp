#include"mpublic.h"
#include"_ooci.h"
using namespace idc;

struct st_arg{
    char connstr[101];     // 数据库的连接参数。
    char tname[31];        // 待清理的表名。
    char keycol[31];        // 待清理的表的唯一键字段名。
    char where[1001];    // 待清理的数据需要满足的条件。
    int    maxcount;        // 执行一次SQL删除的记录数。
    char starttime[31];   // 程序运行的时间区间。
    int  timeout;             // 本程序运行时的超时时间。
    char pname[51];      // 本程序运行时的程序名。
}starg;

void help();
clogfile logfile;
void EXIT(int sig);
bool _xmltoarg(const char *strxmlbuffer);
connection conn;
bool _deletetable();

int main(int argc,char* argv[]){
    if(argc!=3){
        help();
        return -1;
    }
    if(logfile.open(argv[1])==false){
        cout<<"logfile.open failed.\n";
        EXIT(-1);
    }
    if(_xmltoarg(argv[2])==false){EXIT(-1);}//解析的数据放到starg结构体中
    _deletetable();

    return 0;
}

void help()
{
    printf("Using:/project/tools/bin/deletetable logfilename xmlbuffer\n\n");

    printf("Sample:/A/tools/bin/procctl 3600 /A/tools/bin/deletetable /A/log/idc/deletetable_ZHOBTMIND1.log "\
                         "\"<connstr>idc/idcpwd@snorcl11g_5</connstr><tname>T_ZHOBTMIND1</tname>"\
                         "<keycol>rowid</keycol><where>where datetime<sysdate-0.03</where>"\
                         "<maxcount>10</maxcount><starttime>22,23,00,01,02,03,04,05,06,13</starttime>"\
                         "<timeout>120</timeout><pname>deletetable_ZHOBTMIND1</pname>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于清理表中的数据。\n");

    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("tname       待清理数据表的表名。\n");
    printf("keycol      待清理数据表的唯一键字段名，可以用记录编号，如keyid，建议用rowid，效率最高。\n");
    printf("where       待清理的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("maxcount    执行一次SQL语句删除的记录数，建议在100-500之间。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间不运行。"\
                                "如果starttime为空，本参数将失效，只要本程序启动就会执行数据清理，"\
                                "为了减少对数据库的压力，数据清理一般在业务最闲的时候时进行。\n");
    printf("timeout     本程序的超时时间，单位：秒，建议设置120以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d\n\n",sig);
    exit(0);
}

bool _xmltoarg(const char *strxmlbuffer){
    memset(&starg,0,sizeof(st_arg));

    getxmlbuffer(strxmlbuffer,"connstr",starg.connstr,100);
    if (strlen(starg.connstr)==0) { logfile.write("connstr is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"tname",starg.tname,30);
    if (strlen(starg.tname)==0) { logfile.write("tname is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"keycol",starg.keycol,30);
    if (strlen(starg.keycol)==0) { logfile.write("keycol is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"where",starg.where,1000);
    if (strlen(starg.where)==0) { logfile.write("where is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"starttime",starg.starttime,30);

    getxmlbuffer(strxmlbuffer,"maxcount",starg.maxcount);
    if (starg.maxcount==0) { logfile.write("maxcount is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"timeout",starg.timeout);
    if (starg.timeout==0) { logfile.write("timeout is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"pname",starg.pname,50);
    if (strlen(starg.pname)==0) { logfile.write("pname is null.\n"); return false; }

    return true;
}

bool _deletetable()
{
    if (!conn.isopen()) {
        logfile.write("数据库连接未打开，尝试重新连接...\n");
        if (conn.connecttodb(starg.connstr,"Simplified Chinese_China.AL32UTF8") != 0) {
            logfile.write("重新连接失败: %s\n", conn.message());
            return false;
        }
        logfile.write("重新连接成功\n");
    }
    //拼凑出sql语句,先找到rowid
    char tmpvalue[21];      // 存放待删除记录的唯一键的值。

    sqlstatement stmt(&conn);
    // select rowid from T_ZHOBTMIND1 where ddatetime<sysdate-1
    stmt.prepare("select %s from %s %s",starg.keycol,starg.tname,starg.where);
    stmt.bindout(1,tmpvalue,20);
    
    //连接数据库，绑定准备参数
    // delete from T_ZHOBTMIND1 where rowid in (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10);
    string strsql = sformat("delete from %s where %s in (",starg.tname,starg.keycol);
    for(int i=1;i<=starg.maxcount;i++){
        strsql=strsql+sformat(":%d,",i);
    }
    delrstr(strsql,',');
    strsql=strsql+')';
    logfile.write("strsql=%s\n",strsql.c_str());

    char keyvalues[starg.maxcount][21];   // 存放唯一键字段的值的数组。

    sqlstatement stmtdel(&conn);
    string l= "delete from T_ZHOBTMIND1 where rowid in (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10)";
    if(stmtdel.prepare(l)!=0){// 准备删除数据的SQL语句
        logfile.write("stmtdel.prepare failed: %s\n", stmtdel.message());
        return false;
    }
    for (int ii=0;ii<starg.maxcount;ii++){
        stmtdel.bindin(ii+1,keyvalues[ii],20);
        logfile.write("stmtdel.bindin(%d,%s)\n",ii+1,keyvalues[ii]);
        }
        

    if (stmtdel.execute()!=0)                      // 执行提取数据的SQL语句。
    {
        logfile.write("stmtdel.execute() failed.\n%s\n%s\n",stmtdel.sql(),stmtdel.message()); return false;
    }

    //从文件中获取一行数据，放在临时数组中
    //如果临时数组的记录数大于maxcount,执行一次删除数据的sql语句
    //如果临时数组中还有数据，执行一次删除语句
    return true;
}
