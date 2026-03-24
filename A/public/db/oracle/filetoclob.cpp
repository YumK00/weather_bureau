#include"_ooci.h"
using namespace idc;
#include<iostream>
using std::cout;

int main(){
    connection conn;
    if(conn.connecttodb("scott/111111","Simplified Chinese_China.AL32UTF8")!=0){
        cout<<"failed "<<conn.rc()<<","<<conn.message();
        return -1;
    }
    cout<<"connect ok\n";
    sqlstatement stmt(&conn);
    //stmt.prepare("insert into t_girl(id,name,memo1) values(201,'冰',empty_clob())"); 
    stmt.prepare("update t_girl set memo1=empty_clob() where id=1");
    if (stmt.execute() != 0)
    {
        printf("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return -1;
    }

    stmt.prepare("select memo1 from t_girl where id=1 for update");
    stmt.bindclob();
    if (stmt.execute() != 0)
    {
        printf("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return -1;
    }

    // 获取一条记录，一定要判断返回值，0-成功，1403-无记录，其它-失败。
    if (stmt.next() != 0) return 0;

    // 把磁盘文件memo_in.txt的内容写入CLOB字段，一定要判断返回值，0-成功，其它-失败。
    if (stmt.filetolob("/A/public/db/oracle/1.txt") != 0)
    {
        printf("stmt.filetolob() failed.\n%s\n",stmt.message()); return -1;
    }

    printf("文本文件已存入数据库的CLOB字段中。\n");


    conn.commit();
    return 0;
}