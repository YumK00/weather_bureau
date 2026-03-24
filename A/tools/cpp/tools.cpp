#include "tools.h"

ctcols::ctcols()
{
    init();
}

void ctcols::init()
{
    m_vallcols.clear();
    m_vpkcols.clear();
    m_pkcols.clear();
    m_allcols.clear();
}

bool ctcols::allcols(connection &conn, char *tablename)
{
    m_vallcols.clear();
    m_allcols.clear(); 
    st_columns stcolumns;
    sqlstatement stmt(&conn);
    stmt.prepare("select lower(column_name),lower(data_type),data_length from USER_TAB_COLUMNS\
            where table_name=upper(:1) order by column_id");
    stmt.bindin(1,tablename,30);
    stmt.bindout(1,stcolumns.colname,30);
    stmt.bindout(2,stcolumns.datatype,30);
    stmt.bindout(3,stcolumns.collen);
    if(stmt.execute()!=0)   return false;
    while (true)
    {
        memset(&stcolumns,0,sizeof(struct st_columns));
  
        if (stmt.next()!=0) break;

        // 列的数据类型，分为char、date和number三大类。
        // 如果业务有需要，可以修改以下的代码，增加对更多数据类型的支持。

        // 各种字符串类型，rowid当成字符串处理。
        if (strcmp(stcolumns.datatype,"char")==0)            strcpy(stcolumns.datatype,"char");
        if (strcmp(stcolumns.datatype,"nchar")==0)          strcpy(stcolumns.datatype,"char");
        if (strcmp(stcolumns.datatype,"varchar2")==0)     strcpy(stcolumns.datatype,"char");
        if (strcmp(stcolumns.datatype,"nvarchar2")==0)   strcpy(stcolumns.datatype,"char");
        if (strcmp(stcolumns.datatype,"rowid")==0)        { strcpy(stcolumns.datatype,"char");  stcolumns.collen=18; }

        // 日期时间类型。yyyymmddhh24miss
        if (strcmp(stcolumns.datatype,"date")==0)             stcolumns.collen=14; 
    
        // 数字类型。
        if (strcmp(stcolumns.datatype,"number")==0)      strcpy(stcolumns.datatype,"number");
        if (strcmp(stcolumns.datatype,"integer")==0)       strcpy(stcolumns.datatype,"number");
        if (strcmp(stcolumns.datatype,"float")==0)           strcpy(stcolumns.datatype,"number");  

        // 如果字段的数据类型不在上面列出来的中，忽略它。
        if ( (strcmp(stcolumns.datatype,"char")!=0) &&
             (strcmp(stcolumns.datatype,"date")!=0) &&
             (strcmp(stcolumns.datatype,"number")!=0) ) continue;

        // 如果字段类型是number，把长度设置为22。
        if (strcmp(stcolumns.datatype,"number")==0) stcolumns.collen=22;

        m_allcols = m_allcols + stcolumns.colname + ",";        // 拼接全部字段字符串。

        m_vallcols.push_back(stcolumns);                                 // 把字段信息放入容器中。
    }

    // 删除m_allcols最后一个多余的逗号。
    if (stmt.rpc()>0) delrstr(m_allcols,',');            // obtid,ddatetime,....,keyid

    return true;
}

bool ctcols::pkcols(connection &conn, char *tablename)
{
    m_pkcols.clear();
    m_vpkcols.clear();
    st_columns stcolumns;
    memset(&stcolumns,0,sizeof(st_columns));
    sqlstatement stmt(&conn);
    stmt.prepare("select lower(column_name),position from USER_CONS_COLUMNS\
         where table_name=upper(:1)\
           and constraint_name=(select constraint_name from USER_CONSTRAINTS\
                               where table_name=upper(:2) and constraint_type='P')\
         order by position");
        stmt.bindin(1,tablename,30);
        stmt.bindin(2,tablename,30);
        stmt.bindout(1,stcolumns.colname,30);
        stmt.bindout(2,stcolumns.pkseq);
        if(stmt.execute()!=0)   return false;
        int count = 0;
        while(true){
            memset(&stcolumns,0,sizeof(st_columns));
            if(stmt.next()!=0)      break;
            m_pkcols= m_pkcols+stcolumns.colname+",";
            m_vpkcols.push_back(stcolumns);
            count++;
        }
        //logfile.write("DEBUG: 总共找到 %d 个主键字段\n", count);
        //logfile.write("DEBUG: m_pkcols = %s\n", m_pkcols.c_str());
        if (stmt.rpc()>0) delrstr(m_pkcols,',');
        // 更新m_vallcols中的pkseq成员（是否为主键，如果列是主键的字段，存放主键字段的顺序，从1开始，不是主键取值0）。
        for (auto &aa : m_vpkcols) {
            for (auto &bb : m_vallcols) {
                if (strcmp(aa.colname,bb.colname)==0) {
                    bb.pkseq=aa.pkseq; 
                    //logfile.write("DEBUG: 设置字段 %s 的 pkseq = %d\n", 
                    //             bb.colname, bb.pkseq);
                    break;
                }
            }
        }
    return true;
}
