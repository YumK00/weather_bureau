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
    stmt.prepare("select memo1 from t_girl where id = 1");
    stmt.bindclob();
     if (stmt.execute()!=0)
    {
        printf("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return -1;
    }
    if (stmt.next() != 0) return 0;
    if (stmt.lobtofile("/A/public/db/oracle/2.txt") != 0)
    {
        printf("stmt.filetolob() failed.\n%s\n",stmt.message()); return -1;
    }

    printf("文本文件已存入文件中。\n");


    conn.commit();
    return 0;
}