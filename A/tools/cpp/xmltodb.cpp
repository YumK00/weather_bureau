//#include "mpublic.h"
//#include "_ooci.h"
#include"tools.h"
using namespace idc;

cpactive pactive;

void _help();

ctimer timer;//处理每个文件所用的时间计时器
int totlecount,insertcount,updatecount;//总，插入，更细信息的条数
bool _xmltoarg(const char *strxmlbuffer);
void EXIT(int sig);
bool _xmltodb();

struct st_arg {
    // 数据库连接参数，格式：username/passwd@tnsname
    char connstr[128];
    // 数据库字符集（需与数据源一致，避免中文乱码）
    char charset[64];
    // 数据入库的参数配置文件名称
    char inifilename[256];
    // 待入库 XML 文件存放目录
    char xmlpath[256];
    // XML 文件入库后的备份目录
    char xmlpathbak[256];
    // 入库失败的 XML 文件存放目录
    char xmlpatherr[256];
    // 扫描 xmlpath 目录的时间间隔（2-30 秒）
    int timetvl;
    // 程序超时时间（建议 30 秒以上）
    int timeout;
    // 程序名称
    char pname[128];
} starg;

// 数据入库参数的结构体。
struct st_xmltotable{
    char filename[256];// xml文件的匹配规则，用逗号分隔。
    char tname[128];// 待入库的表名。
    int uptbz;// 更新标志：1-更新；2-不更新。
    char execsql[256];// 处理xml文件之前，执行的SQL语句。
}stxmltotable;

vector<st_xmltotable>vxmltotable;

// 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中。
bool loadxmltotable();


clogfile logfile;
connection conn;
//xml文件处理的子函数，由bool _xmltodb();调用，参数列表 全路径名称，单个文件名称
int _xmltodb(const string & fullname,const string &filename);
bool findxmltotable(const string &filename);

ctcols tcols;
/*struct st_columns
{
    char colname[31];
    char datatype[31];
    int collen;
    int pkseq;
}stcolumns;
vector<struct st_columns>m_vallcols;//存放全部字段信息的容器。
vector<struct st_columns>m_vpkcols;//存放主键字段信息的容器。
string m_allcols;//全部的字段名列表，以字符串存放，中间用半角的逗号分隔。
//obtid,to_char(datetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,keyid

string m_pkcols;//主键字段名列表，以字符串存放，中间用半角的逗号分隔。
bool allcols(connection &conn,char* tablename);
bool pkcols(connection &conn,char* tablename);*/

string insertsql;
string updatesql;
void crtsql();

//存放<obtid>58015</obtid><cityname>砀山</cityname>...<lon>11620</lon><height>442</height><endl/>
vector<string>vcolvalue;

sqlstatement stmtinsert,stmtupdate;
void preparesql();

bool execsql();//检查stxmltotable.execsql是否为空，不为空则执行

void splitbuffer(const string &buffer);//解析xml语句，存放在已绑定的vcolvalue数组里

bool xmltopathbak(const string &fullfilename,const string &xmlpath,const string &xmlpathbak);//入库成功后，把源文件移动到bak文件中

int main(int argc,char* argv[]){
    if(argc!=3){_help();EXIT(-1);}

    if(logfile.open(argv[1])==false){
        cout<<"logfile.open failed.\n";
        EXIT(-1);
    }
    if(_xmltoarg(argv[2])==false){EXIT(-1);}//解析的数据放到starg结构体中
    pactive.addpinfo(starg.timeout,starg.pname);
    _xmltodb();
    return 0; 
}

void _help(){
    printf("Using:/A/tools/bin/xmltodb logfilename xmlbuffer\n\n");

    printf("Sample:/A/tools/bin/procctl 10 /A/tools/bin/xmltodb /A/log/idc/xmltodb_vip.log "\
              "\"<connstr>idc/idcpwd</connstr><charset>Simplified Chinese_China.AL32UTF8</charset>"\
              "<inifilename>/A/idc/ini/xmltodb.xml</inifilename>"\
              "<xmlpath>/A/idcdata/xmltodb/vip</xmlpath><xmlpathbak>/A/idcdata/xmltodb/vipbak</xmlpathbak>"\
              "<xmlpatherr>/A/idcdata/xmltodb/viperr</xmlpatherr>"\
              "<timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_vip</pname>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于把xml文件入库到Oracle的表中。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("inifilename 数据入库的参数配置文件。\n");
    printf("xmlpath     待入库xml文件存放的目录。\n");
    printf("xmlpathbak  xml文件入库后的备份目录。\n");
    printf("xmlpatherr  入库失败的xml文件存放的目录。\n");
    printf("timetvl     扫描xmlpath目录的时间间隔（执行入库任务的时间间隔），单位：秒，视业务需求而定，2-30之间。\n");
    printf("timeout     本程序的超时时间，单位：秒，视xml文件大小而定，建议设置30以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

//把xml文件入库到Oracle业务的表中主函数
bool _xmltodb()
{
    cdir dir;
    int  count=31;
    while(true){
        //读xnmtodb.xml文件放到循环里面，一段时间更新一次，这样就可以添加新的规则
        if(count>30){
            if(loadxmltotable()==false) 
                return false;
            count=0;
            
            }
        else{
            count++;
        }

        if (dir.opendir(starg.xmlpath,"*.xml",10000,false,true)==false){
            logfile.write("dir.opendir(%s) failed.n",starg.xmlpath); 
            return false;
        }
        if(conn.isopen()==false){
            if(conn.connecttodb(starg.connstr,starg.charset)!=0){
            logfile.write("conn.connecttodb %s(%s)failed.%d\n",starg.connstr,starg.charset,conn.rc());
                return false;
            }
        logfile.write("connect db(%s) ok.\n",starg.connstr);
        }
        while (true){
            //读取目录，得到一个xml文件。
            if (dir.readdir()==false) {
                //logfile.write("dir.readdir failed.\n");
                break;
                }
            logfile.write("处理文件%s...\n",dir.m_ffilename.c_str());
            //处理xml文件的子函数。
            int ret = _xmltodb(dir.m_ffilename,dir.m_filename);
            pactive.uptatime();
            if(ret==0){
                logfile << "ok(" << stxmltotable.tname << ",总数=" << totlecount << ",插入=" << insertcount
                           << ",更新=" << updatecount << "，耗时=" << timer.elapsed() <<").\n";
                if(xmltopathbak(dir.m_ffilename,starg.xmlpath,starg.xmlpathbak)==false) return false;
            }
            // 1-入库参数不正确；3-待入库的表不存在；4-执行入库前的SQL语句失败。把xml文件移动到错误目录。
            if(ret==1||ret==3||ret==4){
                if(xmltopathbak(dir.m_ffilename,starg.xmlpath,starg.xmlpatherr)==false) return false;
            }
             // 2-数据库错误，函数返回，程序将退出。
             if(ret==2){
                logfile.write("database has issue. exit\n");
                return false;
             }
             // 5- 打开xml文件失败，函数返回，程序将退出。
            if(ret==5){
                logfile.write("xmlfile has issue.exit\n");
                return false;
             } 
        }
    if(dir.size()==0)
        sleep(starg.timetvl);

    pactive.uptatime();
    }
   
    return true;
}

//读取xnmtodb.xml文件，把数据存到stxmltotable结构体再压缩到vxmltotable容器中
bool loadxmltotable(){
    vxmltotable.clear();
    cifile ifile;
    if(ifile.open(starg.inifilename)==false){
        logfile.write("ifile.open %s failed.\n",starg.inifilename);
        return false;
    }
    string buff;
    while(true){
        if(ifile.readline(buff,"<endl/>")==false)   break;
        memset(&stxmltotable,0,sizeof(st_xmltotable));
        getxmlbuffer(buff,"filename",stxmltotable.filename,100);   // xml文件的匹配规则，用逗号分隔。
        getxmlbuffer(buff,"tname",stxmltotable.tname,30);            // 待入库的表名。
        getxmlbuffer(buff,"uptbz",stxmltotable.uptbz);                   // 更新标志：1-更新；2-不更新。
        getxmlbuffer(buff,"execsql",stxmltotable.execsql,300);       // 处理xml文件之前，执行的SQL语句。
        vxmltotable.push_back(stxmltotable);
    }
    logfile.write("loadxmltotable(%s) ok.\n",starg.inifilename);
    for(auto &aa:vxmltotable){
        logfile.write("vxmltotable.filename=%s.\n",aa.filename);
    }
    logfile<<"\n";
    return true;
}


bool findxmltotable(const string &filename){
   
    for(auto &aa:vxmltotable){
        
        if(matchstr(filename,aa.filename)==true){
            stxmltotable=aa;
            //logfile.write("filename=%s,aa.filename=%s",filename.c_str(),aa.filename);
            return true;
        }  
    } 
    return false;
}

void crtsql()
{
    string keysql;
    string valuesql;
    int sqlnum=1;

    // 拼接插入表的SQL语句。 
    // insert into T_ZHOBTMIND1(obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid) 
    //      values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,SEQ_ZHOBTMIND1.nextval)
    for(auto&aa:tcols.m_vallcols){
        if(strcmp("uptime",aa.colname)==0) continue;
        keysql=keysql+aa.colname+",";
        if(strcmp("date",aa.datatype)==0){
            valuesql= valuesql+sformat("to_date(:%d,yyyymmddhh24miss)",sqlnum)+",";
            sqlnum++;
        }
        else if(strcmp("keyid",aa.colname)==0){
            valuesql = valuesql + sformat("SEQ_%s.nextval",stxmltotable.tname+2) + ",";
        }
        else{
            valuesql= valuesql+sformat(":%d",sqlnum)+",";
            sqlnum++;
        }
    }
    delrstr(keysql,',');
    delrstr(valuesql,',');
    sformat(insertsql,"insert into %s(%s)values(%s)",stxmltotable.tname,keysql.c_str(),valuesql.c_str());
    logfile.write("instertsql = %s.\n",insertsql.c_str());

     // 拼接更新表的SQL语句。
    // update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7 
    //                             where obtid=:8 and ddatetime=to_date(:9,'yyyymmddhh24miss')
    if(stxmltotable.uptbz!=1)   return;
    sqlnum=1;
    //update T_ZHOBTMIND1 set
    updatesql=sformat("update %s set ",stxmltotable.tname);

    //t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7
    for(auto&aa:tcols.m_vallcols){
        if(strcmp("keyid",aa.colname)==0)   continue;
        if(strcmp("uptime",aa.colname)==0){
            updatesql=updatesql+sformat("uptime=sysdate,");
            continue;
        }
        if(strcmp(aa.datatype,"date")==0){
            updatesql=updatesql+sformat("to_date(:%d,yyyymmddhh24mmiss),",sqlnum);
        }
        else{
            updatesql=updatesql+sformat("%s=:%d,",aa.colname,sqlnum);
        }
        sqlnum++;
    }
    delrstr(updatesql,',');

    //where obtid=:8 and ddatetime=to_date(:9,'yyyymmddhh24miss')
    updatesql=updatesql+" where 1=1";//方便后续拼接,这样所有的后续拼接逻辑一样不需要拼where
    for(auto&aa:tcols.m_vallcols){
        if (aa.pkseq==0) continue;
        if(strcmp(aa.datatype,"date")==0){
            updatesql=updatesql+sformat(" and to_date(:%d,yyyymmddhh24mmiss)",sqlnum);
        }
        else{
            updatesql=updatesql+sformat(" and %s=:%d",aa.colname,sqlnum);
        }
        sqlnum++;
    }
    delrstr(updatesql,',');
    logfile.write("updatesql = %s.\n",updatesql.c_str());

    return;
}

void preparesql()
{
    vcolvalue.resize(tcols.m_allcols.size());
    int sqlnum = 1;
    
    // stmtinsert和stmtupdate使用全局的conn对象
    stmtinsert.connect(&conn);  // 确保使用全局的conn对象
    if (stmtinsert.prepare(insertsql.c_str()) != 0)  // 检查prepare是否成功
    {
        logfile.write("stmtinsert.prepare failed: %s\n%s\n", insertsql.c_str(), stmtinsert.message());
        return;
    }
    
    for (int i = 0; i < tcols.m_vallcols.size(); i++)
    {
        if ((strcmp(tcols.m_vallcols[i].colname, "uptime") == 0) ||
            (strcmp(tcols.m_vallcols[i].colname, "keyid") == 0))
            continue;
        stmtinsert.bindin(sqlnum, vcolvalue[i], tcols.m_vallcols[i].collen);
        logfile.write("stmtinsert.bindin(%d, vcolvalue[%d], %d);\n", sqlnum, i, tcols.m_vallcols[i].collen);
        sqlnum++;
    }

    if (stxmltotable.uptbz != 1)
        return;

    sqlnum = 1;
    // 绑定set部分参数
    stmtupdate.connect(&conn);  // 确保使用全局的conn对象
    if (stmtupdate.prepare(updatesql.c_str()) != 0)  // 检查prepare是否成功
    {
        logfile.write("stmtupdate.prepare failed: %s\n%s\n", updatesql.c_str(), stmtupdate.message());
        return;
    }
    
    for (int ii = 0; ii < tcols.m_vallcols.size(); ii++)
    {
        // 如果是主键字段，不需要拼接在set的后面
        if (tcols.m_vallcols[ii].pkseq != 0)
            continue;

        // upttime和keyid这两个字段不需要处理
        if ((strcmp(tcols.m_vallcols[ii].colname, "uptime") == 0) ||
            (strcmp(tcols.m_vallcols[ii].colname, "keyid") == 0))
            continue;

        stmtupdate.bindin(sqlnum, vcolvalue[ii], tcols.m_vallcols[ii].collen);
        logfile.write("stmtupdate.bindin(%d, vcolvalue[%d], %d);\n", sqlnum, ii, tcols.m_vallcols[ii].collen);
        sqlnum++;
    }

    // 绑定where部分的输入参数
    for (int ii = 0; ii < tcols.m_vallcols.size(); ii++)
    {
        // 如果不是主键字段，跳过，只有主键字段才拼接在where的后面
        if (tcols.m_vallcols[ii].pkseq == 0)
            continue;

        stmtupdate.bindin(sqlnum, vcolvalue[ii], tcols.m_vallcols[ii].collen);
        logfile.write("stmtupdate.bindin(%d, vcolvalue[%d], %d);\n", sqlnum, ii, tcols.m_vallcols[ii].collen);
        sqlnum++;
    }

    return;
}
/*
void preparesql()
{
    vcolvalue.resize(tcols.m_allcols.size());
    int sqlnum=1;
    connection conn;
    stmtinsert.connect(&conn);
    if (stmtinsert.prepare(insertsql.c_str()) != 0)  // 检查返回值
        {
            logfile.write("stmtinsert.prepare failed: %s\n%s\n", 
                  insertsql.c_str(), stmtinsert.message());
        return;
        }       
    for(int i=0;i<tcols.m_vallcols.size();i++){
        if ( (strcmp(tcols.m_vallcols[i].colname,"uptime")==0) ||
             (strcmp(tcols.m_vallcols[i].colname,"keyid")==0) ) continue;
        stmtinsert.bindin(sqlnum,vcolvalue[i],tcols.m_vallcols[i].collen);
        logfile.write("stmtinsert.bindin(%d,vcolvalue[%d],%d);\n",sqlnum,i,tcols.m_vallcols[i].collen);
        sqlnum++;
    }

    if(stxmltotable.uptbz!=1)   return;
    sqlnum=1;
    //绑定set部分参数
    for (int ii=0;ii<tcols.m_vallcols.size();ii++)     // 遍历全部字段的容器。
    {
        // 如果是主键字段，不需要拼接在set的后面。
        if (tcols.m_vallcols[ii].pkseq!=0) continue;

        // upttime和keyid这两个字段不需要处理。
        if ( (strcmp(tcols.m_vallcols[ii].colname,"uptime")==0) ||
             (strcmp(tcols.m_vallcols[ii].colname,"keyid")==0) ) continue;

        stmtupdate.bindin(sqlnum,vcolvalue[ii],tcols.m_vallcols[ii].collen);
        logfile.write("stmtupdata.bindin(%d,vcolvalue[%d],%d);\n",sqlnum,ii,tcols.m_vallcols[ii].collen);

        sqlnum++;
    }

    // 绑定where部分的输入参数。
    for (int ii=0;ii<tcols.m_vallcols.size();ii++)     // 遍历全部字段的容器。
    {
        // 如果不是主键字段，跳过，只有主键字段才拼接在where的后面。
        if (tcols.m_vallcols[ii].pkseq==0) continue;

        stmtupdate.bindin(sqlnum,vcolvalue[ii],tcols.m_vallcols[ii].collen);
        logfile.write("stmtuptta.bindin(%d,vcolvalue[%d],%d);\n",sqlnum,ii,tcols.m_vallcols[ii].collen);

        sqlnum++;
    }

    return;
}
*/

bool execsql()
{
    if(sizeof(stxmltotable.execsql)==0) return true;
    sqlstatement stmt(&conn);
    stmt.prepare(stxmltotable.execsql);
    if(stmt.execute()!=0){
        logfile.write("stmt.execute failed.%s,%s\n",stmt.sql(),stmt.message());
        return false;
    }
    return true;
}

void splitbuffer(const string &buffer)
{
    string strtemp;   // 临时变量，存放从xml中解析出来的字段的值。

    for (int ii=0;ii<tcols.m_vallcols.size();ii++)   // 遍历全部字段的容器。
    {
        // 根据字段名，从xml中把数据项的值解析出来，存放在临时变量strtemp中。
        // 用临时变量是为了防止调用移动构造和移动赋值函数改变vcolvalue数组中string的内部地址。
		getxmlbuffer(buffer,tcols.m_vallcols[ii].colname,strtemp,tcols.m_vallcols[ii].collen);

        // 如果是日期时间字段date，提取数字就可以了。 
        // 也就是说，xml文件中的日期时间只要包含了yyyymmddhh24miss就行，可以是任意分隔符。
        if (strcmp(tcols.m_vallcols[ii].datatype,"date")==0)
        {
            picknumber(strtemp,strtemp,false,false);
        }
        else if (strcmp(tcols.m_vallcols[ii].datatype,"number")==0)
        {
            // 如果是数值字段number，提取数字、+-符号和圆点。
            picknumber(strtemp,strtemp,true,true);
        }

        // 如果是字符字段char，不需要任何处理。

        // vcolvalue[ii]=strtemp;               // 不能采用这行代码，会调用移动赋值函数。
        vcolvalue[ii]=strtemp.c_str();
    }

    return;
}

bool xmltopathbak(const string &fullfilename, const string &xmlpath, const string &xmlpathbak)
{
    string filename= fullfilename;
    replacestr(filename,xmlpath,xmlpathbak,false);
    if(renamefile(fullfilename,filename)==false){
        logfile.write("renamefile(%s,%s) failed.\n",fullfilename,filename.c_str()); 
        return false;
    }
    return true;
}

int _xmltodb(const string &fullname, const string &filename)
{
    timer.start();
    totlecount=insertcount=updatecount=0;
    //查找filename是否和idc/ini/xmltodb.xml的文件名匹配规则符合,符合的话把值传到stxmltotable里
    if(findxmltotable(filename)==false){
        logfile.write("matchstr(%s) failed.\n",filename.c_str());
        return 1;
    }
    //此时xml文件的filename，tname，uptbz，execsql存放在stxmltotable里
    //而存放xml数据的文件还是在dir.mfilename里，也就是形参fullname
    //logfile.write("stxmltotable.filename= %s\n",stxmltotable.filename);

    //根据表名查找数据字典，查找入库参数配置文件，找到对应的字段名和主键存入tcols类中
    if(tcols.allcols(conn,stxmltotable.tname)==false){
        logfile.write("allcol failed.\n");
        return 2;
    }
    if(tcols.pkcols(conn,stxmltotable.tname)==false){
        logfile.write("pkcol failed.\n");
        return 2;
    }
//tcols.m_allcols =  "obtid,cityname,provname,lat,lon,height,uptime,keyid"
//tcols.m_vallcols = std::vector of length 8, capacity 8 = {{colname = "obtid", '\000' <repeats 25 times>, datatype = "char", '\000' <repeats 26 times>, collen = 5, 
    // pkseq = 1}, {colname = "cityname", '\000' <repeats 22 times>, datatype = "char\000ar2", '\000' <repeats 22 times>, collen = 30, pkseq = 0}, {
    // colname = "provname", '\000' <repeats 22 times>, datatype = "char\000ar2", '\000' <repeats 22 times>, collen = 30, pkseq = 0}, {
    // colname = "lat", '\000' <repeats 27 times>, datatype = "number", '\000' <repeats 24 times>, collen = 22, pkseq = 0}, {
    // colname = "lon", '\000' <repeats 27 times>, datatype = "number", '\000' <repeats 24 times>, collen = 22, pkseq = 0}, {
    // colname = "height", '\000' <repeats 24 times>, datatype = "number", '\000' <repeats 24 times>, collen = 22, pkseq = 0}, {
    // colname = "uptime", '\000' <repeats 24 times>, datatype = "date", '\000' <repeats 26 times>, collen = 14, pkseq = 0}, {
    // colname = "keyid", '\000' <repeats 25 times>, datatype = "number", '\000' <repeats 24 times>, collen = 22, pkseq = 0}}
//tcols.m_vpkcols = std::vector of length 1, capacity 1 = {{colname = "obtid", '\000' <repeats 25 times>, datatype = '\000' <repeats 30 times>, collen = 0, pkseq = 1}}
//tcols.pkcols = {bool (ctcols * const, idc::connection &, char *)} 0x41589c <ctcols::pkcols(idc::connection&, char*)>

//根据查找到的字段名allcols和主键vpkcols,拼接插入和更新表的语句
    if(tcols.m_allcols.size()==0)   return 3;

    crtsql();//拼接sql语句

    preparesql();//绑定sql语句到stmtinsert,stmtupdate

    if(execsql()==false)    return 4;//在处理xml文件之前，如果stxmltotable.execsql不为空则执行先命令

    cifile ifile;
    if(ifile.open(fullname)==false){
        conn.rollback();
        return 5;
    }
    string buff;
    ifile.readline(buff);
    while(true){
        if(ifile.readline(buff,"<endl/>")==false)   break;
        totlecount++;
        splitbuffer(buff);//根据表的字段名，从读取的一行数据中解析出每个字段的值。
        if(stmtinsert.execute()!=0){
            if(stmtinsert.rc()==1){
                if(stxmltotable.uptbz==1){
                    if(stmtupdate.execute()!=0){
                        logfile.write("%s",buff.c_str());
                        logfile.write("stmtupdata.execute() failed.\n%s\n%s\n",stmtupdate.sql(),stmtupdate.message());
                        //如果是普通数据错误就不需要返回
                        // 如果是数据库系统出了问题，常见的问题如下，还可能有更多的错误，如果出现了，再加进来。
                        // ORA-03113: 通信通道的文件结尾；ORA-03114: 未连接到ORACLE；ORA-03135: 连接失去联系；ORA-16014：归档失败。
                        if(stmtupdate.rc()==3113||stmtupdate.rc()==3114||stmtupdate.rc()==3135||stmtupdate.rc()==16014) return 2;
                    }
                    else{updatecount++;}
                }
            }
            else{
                logfile.write("%s",buff.c_str());
                logfile.write("stmtinsert.execute() failed.\n%s\n%s\n",stmtinsert.sql(),stmtinsert.message());
                if(stmtinsert.rc()==3113||stmtinsert.rc()==3114||stmtinsert.rc()==3135||stmtinsert.rc()==16014) return 2;
                    }
            
        }
        else{insertcount++;}
    }
    conn.commit();
    return 0;
}

bool _xmltoarg(const char *strxmlbuffer){
    memset(&starg,0,sizeof(st_arg));
    getxmlbuffer(strxmlbuffer,"connstr",starg.connstr,127);
    if(strlen(starg.connstr)==0){
        logfile.write("starg.connstr is null.\n");
        return false;
    }
    getxmlbuffer(strxmlbuffer,"charset",starg.charset,63);
    if(strlen(starg.charset)==0){
        logfile.write("starg.charset is null.\n");
        return false;
    }
    getxmlbuffer(strxmlbuffer,"inifilename",starg.inifilename,255);
    if(strlen(starg.inifilename)==0){
        logfile.write("starg.inifilename is null.\n");
        return false;
    }
    getxmlbuffer(strxmlbuffer,"xmlpath",starg.xmlpath,255);
    if(strlen(starg.xmlpath)==0){
        logfile.write("starg.xmlpath is null.\n");
        return false;
    }
    getxmlbuffer(strxmlbuffer,"xmlpathbak",starg.xmlpathbak,255);
    if(strlen(starg.xmlpathbak)==0){
        logfile.write("starg.xmlpathbak is null.\n");
        return false;
    }
    getxmlbuffer(strxmlbuffer,"xmlpatherr",starg.xmlpatherr,255);
    if(strlen(starg.xmlpatherr)==0){
        logfile.write("starg.xmlpatherr is null.\n");
        return false;
    }
    char timetvl_str[32] = {0};  // 先以字符串读取，再转换为整数
    getxmlbuffer(strxmlbuffer,"timetvl",timetvl_str,31);
    if(strlen(timetvl_str)==0){
        logfile.write("starg.timetvl is null.\n");
        return false;
    }
    starg.timetvl = atoi(timetvl_str);
    if(starg.timetvl < 2 || starg.timetvl > 30){
        logfile.write("starg.timetvl is invalid, must be between 2 and 30.\n");
        return false;
    }
    char timeout_str[32] = {0};
    getxmlbuffer(strxmlbuffer,"timeout",timeout_str,31);
    if(strlen(timeout_str)==0){
        logfile.write("starg.timeout is null.\n");
        return false;
    }
    starg.timeout = atoi(timeout_str);
    if(starg.timeout < 30){
        logfile.write("starg.timeout is invalid, suggest set above 30.\n");
        return false;
    }
    getxmlbuffer(strxmlbuffer,"pname",starg.pname,127);
    if(strlen(starg.pname)==0){
        logfile.write("starg.pname is null.\n");
        return false;
    }
    //logfile.write("All starg parameters check success.\n");
    return true;
}
void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d\n\n",sig);

    exit(0);
}