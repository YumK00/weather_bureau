//客户端
#include "mpublic.h"
using namespace idc;
ctcpclient client;

    bool denglu();
    bool chaxunyue();
    bool zhuanzhang();

string sendbuff,recebuff;

int main(int argc,char*argv[]){
    if(argc!=3){
        cout<<"/A/tools/cpp/demo/demo 192.168.5.5 5005";
        return -1;
    }
    
    if(client.connect(argv[1],atoi(argv[2]))==false){
        cout<<"tcpclient.connect() failed.\n";
        return -1;
    }
    denglu();
    chaxunyue();
    zhuanzhang();

    return 0;
}

bool denglu(){
    sendbuff="<bizid>1</bizid><username>13922200001</username><password>123456</password>";
    if(client.write(sendbuff)==false){
        printf("客户端denglufasong失败\n");
        return false;
    }
    cout<<"发送成功："<<sendbuff<<endl;
    if(client.read(recebuff)==false){
        printf("客户端denglujieshou失败\n");
        return false;
    }
    cout<<"接收成功："<<recebuff<<endl;
    return true;
}
bool chaxunyue(){
    sendbuff="<bizid>2</bizid><cardid>6262000000001</cardid>";
    if(client.write(sendbuff)==false){
        printf("客户端chayunyuefasong失败\n");
        return false;
    }
    cout<<"发送成功："<<sendbuff<<endl;
    if(client.read(recebuff)==false){
        printf("客户chayuejieshou失败\n");
        return false;
    }
    cout<<"接收成功："<<recebuff<<endl;
    return true;
}
bool zhuanzhang(){

    sendbuff="<bizid>3</bizid><cardid1>6262000000001</cardid1><cardid2>6262000000001</cardid2><je>100.8</je>";
    if (client.write(sendbuff)==false)
    {
        printf("tcpclient.write() failed.\n"); return false;
    }
    cout << "发送：" << sendbuff << endl;

    if (client.read(recebuff,10)==false)
    {
        printf("tcpclient.read() failed.\n"); return false;
    }
    cout << "接收：" << recebuff << endl;

    return true;
}
