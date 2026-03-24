#ifndef TOOLS_H
#define TOOLS_H

#include"mpublic.h"
#include"_ooci.h"
using namespace idc;

class ctcols{
    private:
        struct st_columns
        {
            char colname[31];
            char datatype[31];
            int collen;
            int pkseq;
        };
    public:
        vector<struct st_columns>m_vallcols;//存放全部字段信息的容器。
        vector<struct st_columns>m_vpkcols;//存放主键字段信息的容器。

        string m_allcols;//全部的字段名列表，以字符串存放，中间用半角的逗号分隔。
        //obtid,to_char(datetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,keyid

        string m_pkcols;//主键字段名列表，以字符串存放，中间用半角的逗号分隔。

        ctcols();
        void init();

        bool allcols(connection &conn,char* tablename);
        bool pkcols(connection &conn,char* tablename);
    
};

#endif