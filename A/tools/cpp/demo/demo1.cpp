#include "mpublic.h"
using namespace idc;

ctcpserver server;
clogfile logfile;
void fatherexit(int sig);
void childexit(int sig);
string getbuff,putbuff;

int main(int argc,char*argv[]){
    if(argc!=3){
        cout<<"./demo1 5005 /A/log/demo1.log\n";
        return -1;
    }
    //closeioandsignal(true);
    signal(2,fatherexit);
    signal(15,fatherexit);
    if(logfile.open(argv[2])==false){
        cout<<"logfile.open failed\n";
        return false;   
    }

    if(server.initserver(atoi(argv[1]))==false){
        logfile.write("server.initserver(%s) failed.",argv[1]);
        return -1;
    }
    while (true){
        if(server.accept()==false){
            logfile.write("server.accept failed.");
            fatherexit(0);
        }
        printf("客户端%s已连接\n",server.getid());
        if(fork()>0){
            server.closeconnectfd();
            continue; 
        }
        signal(2,childexit);
        signal(15,childexit);
        server.closelistenfd();

        while(true){
            // 子进程与客户端进行通讯，处理业务。
            if (server.read(getbuff)==false)
            {
                logfile.write("tcpserver.read() failed.\n"); childexit(0);
            }
            logfile.write("接收：%s\n",getbuff.c_str());

            int bizid;
            getxmlbuffer(getbuff,"bizid",bizid);
            if(bizid==1){
                string name,passwd;
                getxmlbuffer(getbuff,"username",name);
                getxmlbuffer(getbuff,"password",passwd);
                if(name=="13922200001"&&passwd=="123456"){
                    putbuff="登陆成功\n";
                }
                else{
                    putbuff=="登录失败\n";
                }
                
            }   
            if(bizid==2){
                string cardid;
                getxmlbuffer(getbuff,"cardid",cardid);  // 获取卡号。

                // 假装操作了数据库，得到了卡的余额。

                putbuff="<retcode>0</retcode><ye>128.83</ye>";
            }   
            if(bizid==3){
                sleep(12);
                string cardid1,cardid2;
                getxmlbuffer(getbuff,"cardid1",cardid1);
                getxmlbuffer(getbuff,"cardid2",cardid2);
                double je;
                getxmlbuffer(getbuff,"je",je);

                // 假装操作了数据库，更新了两个账户的金额，完成了转帐操作。

                if ( je<100 )
                    putbuff="<retcode>0</retcode><message>成功。</message>";
                else
                putbuff="<retcode>-1</retcode><message>余额不足。</message>";
            }      
            if (server.write(putbuff)==false)
            {
                logfile.write("tcpserver.send() failed.\n"); childexit(0);
            }
            logfile.write("发送：%s\n",putbuff.c_str());
        }

    }
}

void fatherexit(int sig)
{
    signal(2,SIG_IGN);
    signal(15,SIG_IGN);
    logfile.write("父进程退出");
    server.closelistenfd();
    kill(0,15);
    exit(0);
}

void childexit(int sig)
{
    signal(2,SIG_IGN);
    signal(15,SIG_IGN);
    logfile.write("子进程退出");
    server.closeconnectfd();
    kill(0,15);
    exit(0);
}
