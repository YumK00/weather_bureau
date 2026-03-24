#ifndef MPUBLIC_H_
#define MPUBLIC_H_
#include "cmpublic.h"

using namespace std;

namespace idc
{
    // 删除str左边的cc,默认是空格
    char *dellstr(char *str, const char cc = ' ');
    string &dellstr(string &str, const char cc = ' ');
    // 右边
    char *delrstr(char *str, const char cc = ' ');
    string &delrstr(string &str, const char cc = ' ');
    // 两边
    char *dellrstr(char *str, const char cc = ' ');
    string &dellrstr(string &str, const char cc = ' ');

    char *toupper(char *str);
    string &toupper(string &str);
    char *tolower(char *str);
    string &tolower(string &str);

    // 字符串替换函数。
    // 在字符串str中，如果存在字符串str1，就替换为字符串str2。
    // str：待处理的字符串。
    // str1：旧的内容。
    // str2：新的内容。
    // bloop：是否循环执行替换。
    // 注意：
    // 1、如果str2比str1要长，替换后str会变长，所以必须保证str有足够的空间，否则内存会溢出（C++风格字符串不存在这个问题）。
    // 2、如果str2中包含了str1的内容，且bloop为true，这种做法存在逻辑错误，replacestr将什么也不做。
    // 3、如果str2为空，表示删除str中str1的内容。
    bool replacestr(char *str, const string &str1, const string &str2, const bool bloop = false);
    bool replacestr(string &str, const string &str1, const string &str2, const bool bloop = false);

    char*     picknumber(const string &src,char *dest,const bool bsigned=false,const bool bdot=false);
    string& picknumber(const string &src,string &dest,const bool bsigned=false,const bool bdot=false);
    string    picknumber(const string &src,const bool bsigned=false,const bool bdot=false);

    bool matchstr(const string &str,const string &rules);

    // ccmdstr类用于拆分有分隔符的字符串。
// 字符串的格式为：字段内容1+分隔符+字段内容2+分隔符+字段内容3+分隔符+...+字段内容n。
// 例如："messi,10,striker,30,1.72,68.5,Barcelona"，这是足球运动员梅西的资料。
// 包括：姓名、球衣号码、场上位置、年龄、身高、体重和效力的俱乐部，字段之间用半角的逗号分隔。
class ccmdstr{
    private:
        vector<string>m_cmdstr;
        ccmdstr(const ccmdstr&)=delete;
        ccmdstr &operator=(const ccmdstr&)=delete;
    public:
        ccmdstr(){};
        ccmdstr(const string& buffer,const string&rule,const bool isDelSpace=false){splittocmd(buffer,rule,isDelSpace);}
        void splittocmd(const string& buffer,const string&rules,const bool isDelSpace=false);
        int size() const { return m_cmdstr.size(); }
        int cmdcount() const { return m_cmdstr.size(); } 
        const string& operator[](int i)const {return m_cmdstr[i];}//const重载版本的返回值和函数本体都需要const
        string& operator[](int i){return m_cmdstr[i];}

    // 从m_cmdstr容器获取字段内容。
    // ii：字段的顺序号，类似数组的下标，从0开始。
    // value：传入变量的地址，用于存放字段内容。
    // 返回值：true-成功；如果ii的取值超出了m_cmdstr容器的大小，返回失败。
    bool getvalue(const int ii,string &value,const int ilen=0) const;      // C++风格字符串。视频中没有第三个参数，加上第三个参数更好。
    bool getvalue(const int ii,char *value,const int ilen=0) const;          // C风格字符串，ilen缺省值为0-全部长度。 
    bool getvalue(const int ii,int  &value) const;                                    // int整数。
    bool getvalue(const int ii,unsigned int &value) const;                     // unsigned int整数。
    bool getvalue(const int ii,long &value) const;                                  // long整数。
    bool getvalue(const int ii,unsigned long &value) const;                  // unsigned long整数。
    bool getvalue(const int ii,double &value) const;                              // 双精度double。
    bool getvalue(const int ii,float &value) const;                                  // 单精度float。
    bool getvalue(const int ii,bool &value) const;                                  // bool型。

    ~ccmdstr(); // 析构函数。

    friend ostream& operator<<(ostream& os,const ccmdstr &p);   
};
// 解析xml格式字符串的函数族。
    // xml格式的字符串的内容如下：
    // <filename>/tmp/_public.h</filename><mtime>2020-01-01 12:20:35</mtime><size>18348</size>
    // <filename>/tmp/_public.cpp</filename><mtime>2020-01-01 10:10:15</mtime><size>50945</size>
    // xmlbuffer：待解析的xml格式字符串。
    // fieldname：字段的标签名。
    // value：传入变量的地址，用于存放字段内容，支持bool、int、insigned int、long、
    //       unsigned long、double和char[]。
    // 注意：当value参数的数据类型为char []时，必须保证value数组的内存足够，否则可能发生内存溢出的问题，
    //           也可以用ilen参数限定获取字段内容的长度，ilen的缺省值为0，表示不限长度。
    // 返回值：true-成功；如果fieldname参数指定的标签名不存在，返回失败。
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,string &value,const int ilen=0); 
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,char *value,const int ilen=0);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,bool &value);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,int  &value);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,unsigned int &value);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,long &value);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,unsigned long &value);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,double &value);
    bool getxmlbuffer(const string &xmlbuffer,const string &fieldname,float &value);

template<typename...T>
bool sformat(string&str,const char*fmt,T...t){
    int len = snprintf(nullptr,0,fmt,t...);
    if(len<0) return false;
    if(len==0){
        str.clear();
         return true;
         }
    str.resize(len);
    snprintf(&str[0],len+1,fmt,t...);
    return true;
}
template<typename...T>
string sformat(const char*fmt,T...t){
    string str;
    int len = snprintf(nullptr,0,fmt,t...);
    if(len<0) return str;
    if(len==0){
        str.clear();
        return str;
    }
    str.resize(len);
    snprintf(&str[0],len+1,fmt,t...);
    return str;
}



string& ltime(string &strtime,const string &fmt="",const int timetvl=0);
char *    ltime(char *strtime   ,const string &fmt="",const int timetvl=0);
// 为了避免重载的岐义，增加ltime1()函数。
string    ltime1(const string &fmt="",const int timetvl=0);

// 把整数表示的时间转换为字符串表示的时间。
// ttime：整数表示的时间。
// strtime：字符串表示的时间。
// fmt：输出字符串时间strtime的格式，与ltime()函数的fmt参数相同，如果fmt的格式不正确，strtime将为空。
string& timetostr(const time_t ttime,string &strtime,const string &fmt="");
char*     timetostr(const time_t ttime,char *strtime   ,const string &fmt="");
// 为了避免重载的岐义，增加timetostr1()函数。
string    timetostr1(const time_t ttime,const string &fmt="");

// 把字符串表示的时间转换为整数表示的时间。
// strtime：字符串表示的时间，格式不限，但一定要包括yyyymmddhh24miss，一个都不能少，顺序也不能变。
// 返回值：整数表示的时间，如果strtime的格式不正确，返回-1。
time_t strtotime(const string &strtime);

// 把字符串表示的时间加上一个偏移的秒数后得到一个新的字符串表示的时间。
// in_stime：输入的字符串格式的时间，格式不限，但一定要包括yyyymmddhh24miss，一个都不能少，顺序也不能变。
// out_stime：输出的字符串格式的时间。
// timetvl：需要偏移的秒数，正数往后偏移，负数往前偏移。
// fmt：输出字符串时间out_stime的格式，与ltime()函数的fmt参数相同。
// 注意：in_stime和out_stime参数可以是同一个变量的地址，如果调用失败，out_stime的内容会清空。
// 返回值：true-成功，false-失败，如果返回失败，可以认为是in_stime的格式不正确。
bool addtime(const string &in_stime,char *out_stime    ,const int timetvl,const string &fmt="");
bool addtime(const string &in_stime,string &out_stime,const int timetvl,const string &fmt="");

// 这是一个精确到微秒的计时器。
class ctimer
{
private:
    struct timeval m_start;    // 计时开始的时间点。
    struct timeval m_end;     // 计时结束的时间点。
public:
    ctimer();          // 构造函数中会调用start方法。

    void start();     // 开始计时。

    // 计算已逝去的时间，单位：秒，小数点后面是微秒。
    // 每调用一次本方法之后，自动调用start方法重新开始计时。
    double elapsed();
};



// 根据绝对路径的文件名或目录名逐级的创建目录。
// pathorfilename：绝对路径的文件名或目录名。
// bisfilename：指定pathorfilename的类型，true-pathorfilename是文件名，否则是目录名，缺省值为true。
// 返回值：true-成功，false-失败，如果返回失败，原因有大概有三种情况：
// 1）权限不足；2）pathorfilename参数不是合法的文件名或目录名；3）磁盘空间不足。
bool newdir(const string &pathorfilename,bool bisfilename=true);

//一定要有权限

///////////////////////////////////// /////////////////////////////////////
// 文件操作相关的函数

// 重命名文件，类似Linux系统的mv命令。
// srcfilename：原文件名，建议采用绝对路径的文件名。
// dstfilename：目标文件名，建议采用绝对路径的文件名。
// 返回值：true-成功；false-失败，失败的主要原因是权限不足或磁盘空间不够，如果原文件和目标文件不在同一个磁盘分区，重命名也可能失败。
// 注意，在重命名文件之前，会自动创建dstfilename参数中包含的目录。
// 在应用开发中，可以用renamefile()函数代替rename()库函数。
bool renamefile(const string &srcfilename,const string &dstfilename);

// 复制文件，类似Linux系统的cp命令。
// srcfilename：原文件名，建议采用绝对路径的文件名。
// dstfilename：目标文件名，建议采用绝对路径的文件名。
// 返回值：true-成功；false-失败，失败的主要原因是权限不足或磁盘空间不够。
// 注意：
// 1）在复制文件之前，会自动创建dstfilename参数中的目录名。
// 2）复制文件的过程中，采用临时文件命名的方法，复制完成后再改名为dstfilename，避免中间状态的文件被读取。
// 3）复制后的文件的时间与原文件相同，这一点与Linux系统cp命令不同。
bool copyfile(const string &srcfilename,const string &dstfilename);

// 获取文件的大小。
// filename：待获取的文件名，建议采用绝对路径的文件名。
// 返回值：如果文件不存在或没有访问权限，返回-1，成功返回文件的大小，单位是字节。
int filesize(const string &filename);

// 获取文件的时间。
// filename：待获取的文件名，建议采用绝对路径的文件名。
// mtime：用于存放文件的时间，即stat结构体的st_mtime。
// fmt：设置时间的输出格式，与ltime()函数相同，但缺省是"yyyymmddhh24miss"。
// 返回值：如果文件不存在或没有访问权限，返回false，成功返回true。
bool filemtime(const string &filename,char *mtime    ,const string &fmt="yyyymmddhh24miss");
bool filemtime(const string &filename,string &mtime,const string &fmt="yyyymmddhh24miss");

// 重置文件的修改时间属性。
// filename：待重置的文件名，建议采用绝对路径的文件名。
// mtime：字符串表示的时间，格式不限，但一定要包括yyyymmddhh24miss，一个都不能少，顺序也不能变。
// 返回值：true-成功；false-失败，失败的原因保存在errno中。
bool setmtime(const string &filename,const string &mtime);


class cdir{
    private:
    vector<string>m_filelist;
    int m_pos;
    string m_fmt;

    cdir (const cdir&)=delete;
    cdir &operator=(const cdir&)=delete;
    public:
    // /project/public/_public.h
    string m_dirname;        // 目录名，例如：/project/public
    string m_filename;       // 文件名，不包括目录名，例如：_public.h
    string m_ffilename;      // 绝对路径的文件，例如：/project/public/_public.h
    int m_filesize;          // 文件的大小，单位：字节。
    string m_mtime;           // 文件最后一次被修改的时间，即stat结构体的st_mtime成员。
    string m_ctime;            // 文件生成的时间，即stat结构体的st_ctime成员。
    string m_atime;            // 文件最后一次被访问的时间，即stat结构体的st_atime成员。

    cdir():m_pos(0),m_fmt("yyyymmddhh24miss") {} 

    // 打开目录，获取目录中文件的列表，存放在m_filelist容器中。
    // dirname，目录名，采用绝对路径，如/tmp/root。
    // rules，文件名的匹配规则，不匹配的文件将被忽略。
    // maxfiles，本次获取文件的最大数量，缺省值为10000个，如果文件太多，可能消耗太多的内存。
    // bandchild，是否打开各级子目录，缺省值为false-不打开子目录。
    // bsort，是否按文件名排序，缺省值为false-不排序。
    // 返回值：true-成功，false-失败。
    bool opendir(const string &dirname,const string &rules,const int maxfiles=10000,const bool bandchild=false,bool bsort=false);

    private:
    bool _opendir(const string &dirname,const string &rules,const int maxfiles,const bool bandchild);

    public:
    bool readdir();

    // 设置文件时间的格式，支持"yyyy-mm-dd hh24:mi:ss"和"yyyymmddhh24miss"两种，缺省是后者。
    void setfmt(const string &fmt);

    unsigned int size() { return m_filelist.size(); }

    ~cdir();  // 析构函数。

};

class cofile{
    private:
    fstream fout;
    string m_filename;
    string m_filenametmp;
    cofile(const cofile &)= delete; 
    cofile &operator=(const cofile &)=delete;

    public:
    cofile(){}
    bool isopen() const {return fout.is_open();}

    // 打开文件。
    // filename，待打开的文件名。
    // btmp，是否采用临时文件的方案。
    // mode，打开文件的模式。
    // benbuffer，是否启用文件缓冲区。
     bool open(const string &filename,const bool btmp=true,const ios::openmode mode=ios::out|ios::trunc,const bool benbuffer=true);

    template<typename T>
    cofile &operator<<(const T& t){
        fout<<t;
        return *this;
    }

    template<typename... T>
    bool writeline(const char* s,T... t){
        if(!isopen()){
            return false;
        }
        fout<<sformat(s,t...);
        return fout.good();
    }

// 把二进制数据写入文件。
    bool write(void *buf,int bufsize);

    // 关闭文件，并且把临时文件名改为正式文件名。
    bool closeandrename();

    // 关闭文件，如果有临时文件，则删除它。
    void close();

    ~cofile();
    
};


class cifile{
    private:
    ifstream fin;
    string m_filename;
    cifile(const cifile&)= delete;
    cifile &operator=(const cifile&) =delete;

    public:
    cifile(){}
    bool isopen(){return fin.is_open();}
    bool open(const string&filename,const ios::openmode mode=ios::in);
    // 以行的方式读取文本文件，endbz指定行的结尾标志，缺省为空，没有结尾标志。
    bool readline(string &buf,const string& endbz="");

    int read(void*buf,const int bufsize);
    bool closeandremove();
    void close();
    ~cifile(){close();}
};

class spinlock_mutex{
    private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    
    public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // 忙等待 - 自旋
        }
    }
    
    void unlock() {
        flag.clear(std::memory_order_release);
    }
};

class clogfile{
    private:
    ofstream fout;
    string m_filename;
    bool m_backup;
    int m_maxsize;
    bool m_enbuffer;
    spinlock_mutex m_splock;
    ios::openmode m_mode;

    public:
    clogfile(int maxsize=100):m_maxsize(maxsize){}

    bool open(const string& filename,const std::ios::openmode mode = ios::app,
        const bool backup=true,const bool enbuffer=false);
    
    template<typename T>
    clogfile &operator<<(const T &value){
        backup();
        m_splock.lock();
        fout<<value;
        m_splock.unlock();
        return *this;
    }

    template<typename... T>
    bool write(const char*ch,T...t){
        
        if(!fout.is_open()) return false;
        m_splock.lock();
        if (!fout.good()) {
            cout << "Recovering file stream..." << endl;
            fout = ofstream();  // 重建 ofstream
            fout.open(m_filename, m_mode);
            if (!fout.good()) {
                m_splock.unlock();
                return false;
            }   
        }
        fout << ltime1() << " " << sformat(ch, t...);
        m_splock.unlock();
        return true;
    }

    private:
    bool backup();
    public:
    void close(){fout.clear();}
    ~clogfile(){close();}
};


class ctcpclient{
    private:
    int m_connectfd;//连接用的套接字
    int m_port;
    string m_ip;

    ctcpclient(const ctcpclient&)=delete;
    ctcpclient &operator=(const ctcpclient&)=delete;

    public:
    ctcpclient():m_connectfd(-1),m_ip(""),m_port(0){}
    bool connect(const string& ip,const int port);

    bool write(const string& buffer);
    bool write(const char* buffer,const int size);

    bool read(string& buffer,int itimeout=0);
    bool read(char* buffer,int maxsize,const int itimeout=0);

    bool close(int fd){
        if(m_connectfd>0){
            ::close(m_connectfd);
        }
        m_connectfd=-1;
        m_port=0;
        m_port=0;
        return true;
    }


    ~ctcpclient(){close(m_connectfd);}

};
class ctcpserver{
    private:
    int m_connectfd;
    int m_listenfd;
    int m_port;

    struct sockaddr_in clientfd;//客户端地址
    int m_socklen;//客户端地址的长度
    struct sockaddr_in serverfd;//服务器地址

    ctcpserver(const ctcpserver&)=delete;
    ctcpserver &operator=(const ctcpserver&)=delete;

    public:
    ctcpserver():m_connectfd(-1),m_listenfd(-1){}

    char* getid();

    bool initserver(const int port,const int backlog=5);
    bool accept();
    
    bool write(const string& buffer);
    bool write(const char* buffer,const int size);

    bool read(string &buffer,const int itimeout=0);
    bool read(char* buffer,int maxsize,const int itimeout=0);

    bool closeconnectfd();
    bool closelistenfd();
    ~ctcpserver(){closeconnectfd();closelistenfd();}

};

bool tcpwrite(const int connectfd,const string& buffer);
bool tcpwrite(const int connectfd,const char* buffer,const int size);

bool writen(const int connectfd,const void* buffer,const int size);


bool tcpread(const int connectfd,string& buffer,const int itimeout=0);
bool tcpread(const int connectfd,char* buffer,const int size,const int itimeout=0);

bool readn(const int connectfd,char* buffer,const int size);

void closeioandsignal(bool bcloseio=false);


// 信号量。
class csemp
{
private:
    union semun  // 用于信号量操作的共同体。
    {
      int val;
      struct semid_ds *buf;
      unsigned short  *arry;
    };

    int   m_semid;         // 信号量id（描述符）。

    // 如果把sem_flg设置为SEM_UNDO，操作系统将跟踪进程对信号量的修改情况，
    // 在全部修改过信号量的进程（正常或异常）终止后，操作系统将把信号量恢复为初始值。
    // 如果信号量用于互斥锁，设置为SEM_UNDO。
    // 如果信号量用于生产消费者模型，设置为0。
    short m_sem_flg;

    csemp(const csemp &) = delete;                      // 禁用拷贝构造函数。
    csemp &operator=(const csemp &) = delete;  // 禁用赋值函数。
public:
    csemp():m_semid(-1){}

    // 如果信号量已存在，获取信号量；如果信号量不存在，则创建它并初始化为value。
    // 如果用于互斥锁，value填1，sem_flg填SEM_UNDO。
    // 如果用于生产消费者模型，value填0，sem_flg填0。
    bool init(key_t key,unsigned short value=1,short sem_flg=SEM_UNDO);
    bool wait(short value=-1);    // 信号量的P操作，如果信号量的值是0，将阻塞等待，直到信号量的值大于0。
    bool post(short value=1);     // 信号量的V操作。
    int  getvalue();                       // 获取信号量的值，成功返回信号量的值，失败返回-1。
    bool destroy();                       // 销毁信号量。
    ~csemp();
};



// 进程心跳信息的结构体。
struct st_procinfo
{
    int      pid=0;                      // 进程id。
    char   pname[51]={0};        // 进程名称，可以为空。
    int      timeout=0;              // 超时时间，单位：秒。
    time_t atime=0;                 // 最后一次心跳的时间，用整数表示。
    st_procinfo() = default;     // 有了自定义的构造函数，编译器将不提供默认构造函数，所以启用默认构造函数。
    st_procinfo(const int in_pid,const string & in_pname,const int in_timeout, const time_t in_atime)
                    :pid(in_pid),timeout(in_timeout),atime(in_atime) { strncpy(pname,in_pname.c_str(),50); }
};

// 以下几个宏用于进程的心跳。
#define MAXNUMP     1000     // 最大的进程数量。
#define SHMKEYP    0x5090    // 共享内存的key。
#define SEMKEYP     0x5090     // 信号量的key 两个key可以一样 是两个概念

// 查看共享内存：  ipcs -m
// 删除共享内存：  ipcrm -m shmid
// 查看信号量：      ipcs -s
// 删除信号量：      ipcrm sem semid

// 进程心跳操作类。
class cpactive
{
 private:
     int  m_shmid;                   // 共享内存的id。
     int  m_pos;                       // 当前进程在共享内存进程组中的位置。
     st_procinfo *m_shm;        // 指向共享内存的地址空间。

 public:
     cpactive();  // 初始化成员变量。

     // 把当前进程的信息加入共享内存进程组中。
     bool addpinfo(const int timeout,const string &pname="",clogfile *logfile=nullptr);

     // 更新共享内存进程组中当前进程的心跳时间。
     bool uptatime();

     ~cpactive();  // 从共享内存中删除当前进程的心跳记录。
};



template<typename TT,int MaxLength>
class squeue{
    private:
    bool m_inited;
    TT m_data[MaxLength];
    int m_length;
    int m_head;
    int m_tail;
    squeue(const squeue&)=delete;
    squeue &operator=(const squeue&)=delete;

    public:
    squeue(){init();}
    void init(){
        m_length=0;
        m_head=0;
        m_tail = MaxLength-1;
        memset(m_data,0,sizeof(m_data));
        m_inited=true;
    }
    bool isempty(){
        if(m_length==0)return true;
        return false;
    }
    bool isfull(){
        if(m_length==MaxLength) return true; 
        return false;
    }

    bool push(const TT& value){
        if(isfull())return false;
        m_tail=(m_tail+1)%MaxLength;
        m_data[m_tail]=value;
        m_length++;
        return true;
    }
    bool pop(){
        if(isempty())return false;
        m_head=(m_head+1)%MaxLength;
        m_length--;
        return true;
    }
    int size(){
        return m_length;
    }
    TT& front(){
        return m_data[m_head];
    }
    void printqueu(){
        for (int ii = 0; ii < size(); ii++)
        {
            cout << "m_data[" << (m_head+ii)%MaxLength << "],value=" \
                 << m_data[(m_head+ii)%MaxLength] << endl;
        }
    }  
};


} // namespace

#endif