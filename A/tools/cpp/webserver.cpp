#include "mpublic.h"
#include "_ooci.h"
using namespace idc;

// 设置文件描述符为非阻塞
int set_nonblock(int fd);

int initserver(const int port);

bool getvalue(const string &strget, const string &name, string &value, const int len);

void workmain(connection &conn, const string &recvbuff, string &sendbuff, const string &clientip);

clogfile logfile;

void EXIT(int sig);

struct st_client {
    // 每个客户端需要的信息
    string clientip;      // 客户端IP - 用于日志和权限验证
    int clientatime;      // 活动时间 - 用于超时清理
    string recvbuffer;    // 接收缓冲区 - TCP粘包处理
    string sendbuffer;    // 发送缓冲区 - 大包分片发送
};

struct st_recvmesg {
    // 消息的封装
    int sock = 0;               // 客户端的socket。
    string clientip;            // 客户端IP - 新增字段
    string message;             // 接收/发送的报文。
    
    st_recvmesg(int m_sock, const string &m_clientip, const string &m_message) 
        : sock(m_sock), clientip(m_clientip), message(m_message){}
};

// 主控类
class AA {
    // 队列、锁、线程函数
    // 1. 队列相关 - 用来在线程间传递数据
    queue<shared_ptr<st_recvmesg>> m_rq;     // 接收队列
    mutex m_mutex_rq;                         // 接收队列的锁
    condition_variable m_cond_rq;              // 接收队列的条件变量

    queue<shared_ptr<st_recvmesg>> m_sq;     // 发送队列
    mutex m_mutex_sq;                          // 发送队列的锁
    int m_sendpipe[2] = {0};                   // 通知管道

    // 2. 数据存储 - 保存所有客户端的状态
    unordered_map<int, struct st_client> clientmap;
    mutex m_mutex_clientmap;                   // 存储客户信息到clientmap的锁

    // 3. 控制标志
    atomic_bool m_exit;                        // 退出标志

public:
    int m_recvpipe[2] = {0};                   // 接收线程的退出管道
    
    AA() { 
        if (pipe(m_sendpipe) != 0) {
            logfile.write("创建发送管道失败\n");
        }
        if (pipe(m_recvpipe) != 0) {
            logfile.write("创建接收管道失败\n");
        }
        set_nonblock(m_recvpipe[0]);
        set_nonblock(m_sendpipe[0]);
        m_exit = false;
    }
    
    ~AA() {
        if (m_sendpipe[0] > 0) close(m_sendpipe[0]);
        if (m_sendpipe[1] > 0) close(m_sendpipe[1]);
        if (m_recvpipe[0] > 0) close(m_recvpipe[0]);
        if (m_recvpipe[1] > 0) close(m_recvpipe[1]);
    }

public:
    void inrq(int sock, const string &clientip, const string &message)
    {
        shared_ptr<st_recvmesg> ptr = make_shared<st_recvmesg>(sock, clientip, message);
        {
            lock_guard<mutex> lock(m_mutex_rq);
            m_rq.push(ptr);
        }
        m_cond_rq.notify_one();  // 唤醒一个工作线程
    }
    
    void insq(int sock, const string &message){
        {
            shared_ptr<st_recvmesg> ptr = make_shared<st_recvmesg>(sock, "", message);
            lock_guard<mutex> lock(m_mutex_sq);
            m_sq.push(ptr);
        }
        char c = '1';
        write(m_sendpipe[1], &c, 1);  // 随便写一个数据从管道发出去，让发送线程知道有数据要发送
    }

    void recvfunc(const int listenport){
        int listenfd = initserver(listenport);
        if (listenfd < 0) {
            logfile.write("初始化服务器失败\n");
            return;
        }
        set_nonblock(listenfd);

        int epollfd = epoll_create1(0);
        if (epollfd == -1) {
            perror("epoll_create1 failed");
            close(listenfd);
            return;
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = listenfd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
            perror("epoll_ctl: listenfd");
            close(listenfd);
            close(epollfd);
            return;
        }

        ev.events = EPOLLIN;
        ev.data.fd = m_recvpipe[0];
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, m_recvpipe[0], &ev) == -1) {
            perror("epoll_ctl: recvpipe");
            close(epollfd);
            return;
        }

        struct epoll_event evs[100];  // 返回事件
        logfile.write("接收线程启动，监听端口：%d\n", listenport);
        
        while (!m_exit) {
            int infds = epoll_wait(epollfd, evs, 100, 1000);  // 1秒超时
            
            if (infds < 0) {
                if (errno != EINTR) {
                    logfile.write("接收线程:epoll() failed: %s\n", strerror(errno));
                }
                continue;
            }
            
            for (int ii = 0; ii < infds; ii++) {
                // 如果是新连接
                if (evs[ii].data.fd == listenfd) {
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
                    if (clientfd < 0) continue;
                    
                    set_nonblock(clientfd);
                    string clientip = inet_ntoa(clientaddr.sin_addr);
                    
                    logfile.write("接受新连接:client(socket=%d, ip=%s).\n", clientfd, clientip.c_str());
                    
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
                    
                    // 存储所有连接上来的用户的信息
                    {
                        lock_guard<mutex> lock(m_mutex_clientmap);
                        st_client client;
                        client.clientip = clientip;
                        client.clientatime = time(0);
                        clientmap[clientfd] = client;
                    }
                    continue;
                }
                
                // 如果是退出管道有事件
                if (evs[ii].data.fd == m_recvpipe[0]) {
                    char cc;
                    read(m_recvpipe[0], &cc, 1);
                    logfile.write("程序即将退出.\n");
                    m_exit = true;
                    m_cond_rq.notify_all();
                    break;
                }
                
                // 如果是客户端连接的socket有事件
                if (evs[ii].events & EPOLLIN) {
                    char buff[1024];
                    int bufflen = recv(evs[ii].data.fd, buff, sizeof(buff), 0);
                    
                    // 用接收buff长度判断，如果len<=0则是已经断开
                    if (bufflen <= 0) {
                        logfile.write("客户端断开连接,socket=%d.\n", evs[ii].data.fd);
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, evs[ii].data.fd, NULL);
                        close(evs[ii].data.fd);
                        
                        {
                            lock_guard<mutex> lock(m_mutex_clientmap);
                            clientmap.erase(evs[ii].data.fd);
                        }
                        continue;
                    }
                    
                    logfile.write("接收线程:recv %d,%d bytes\n", evs[ii].data.fd, bufflen);
                    
                    string clientip;
                    {
                        lock_guard<mutex> lock(m_mutex_clientmap);
                        auto it = clientmap.find(evs[ii].data.fd);
                        if (it != clientmap.end()) {
                            it->second.recvbuffer.append(buff, bufflen);
                            it->second.clientatime = time(0);
                            clientip = it->second.clientip;
                            
                            // 检查是否收到完整的HTTP请求（以\r\n\r\n结尾）
                            if (it->second.recvbuffer.length() >= 4 && 
                                it->second.recvbuffer.substr(it->second.recvbuffer.length() - 4) == "\r\n\r\n") {
                                logfile.write("接收线程：接收到了一个完整的请求报文。\n");
                                // 放到接收队列中去，传入clientip
                                inrq(evs[ii].data.fd, clientip, it->second.recvbuffer);
                                it->second.recvbuffer.clear();
                            }
                        }
                    }
                }
                
                // 处理错误事件
                if (evs[ii].events & (EPOLLHUP | EPOLLERR)) {
                    logfile.write("socket=%d 发生错误\n", evs[ii].data.fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, evs[ii].data.fd, NULL);
                    close(evs[ii].data.fd);
                    
                    {
                        lock_guard<mutex> lock(m_mutex_clientmap);
                        clientmap.erase(evs[ii].data.fd);
                    }
                }
            }
        }
        
        close(listenfd);
        close(epollfd);
        logfile.write("接收线程退出\n");
    }

    void workfunc(int id){
        connection conn;
        if (conn.connecttodb("idc/idcpwd@snorcl11g_5", "Simplified Chinese_China.AL32UTF8") != 0) {
            logfile.write("connect database(idc/idcpwd@snorcl11g_5) failed.\n%s\n", conn.message()); 
            return;
        }
        logfile.write("工作线程(%d)启动，数据库连接成功\n", id);
        
        while (!m_exit) {
            shared_ptr<st_recvmesg> ptr;
            {
                unique_lock<mutex> lock(m_mutex_rq);
                while (m_rq.empty() && !m_exit) {
                    m_cond_rq.wait_for(lock, chrono::seconds(1));
                }
                
                if (m_exit && m_rq.empty()) {
                    logfile.write("工作线程(%d)即将退出.\n", id);  
                    return;
                }
                
                if (!m_rq.empty()) {
                    ptr = m_rq.front();
                    m_rq.pop();
                } else {
                    continue;
                }
            }
            
            logfile.write("工作线程(%d)处理请求:sock=%d, clientip=%s, msg=%s\n", 
                         id, ptr->sock, ptr->clientip.c_str(), ptr->message.c_str());
            
            // 处理请求报文的代码
            string sendbuff;
            workmain(conn, ptr->message, sendbuff, ptr->clientip);
            
            // 构造HTTP响应
            string http_response = 
                "HTTP/1.1 200 OK\r\n"
                "Server: webserver\r\n"
                "Content-Type: text/html;charset=utf-8\r\n"
                "Content-Length: " + to_string(sendbuff.size()) + "\r\n"
                "\r\n" + 
                sendbuff;

            // 放入发送队列
            insq(ptr->sock, http_response);
        }
    }

    void sendfunc(){
        int epollfd = epoll_create1(0);
        if (epollfd < 0) {
            logfile.write("发送线程: epoll_create failed\n");
            return;
        }
        
        struct epoll_event ev;
        ev.data.fd = m_sendpipe[0];
        ev.events = EPOLLIN;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, m_sendpipe[0], &ev);
        
        struct epoll_event evs[100];
        logfile.write("发送线程启动\n");
        
        while (!m_exit) {
            int infds = epoll_wait(epollfd, evs, 100, 1000);  // 1秒超时
            
            if (infds <= 0) {
                if (infds < 0 && errno != EINTR) {
                    logfile.write("发送线程:epoll() failed: %s\n", strerror(errno));
                }
                continue;
            }
            
            for (int ii = 0; ii < infds; ii++) {
                if (evs[ii].data.fd == m_sendpipe[0]) {
                    if (m_exit) {
                        logfile.write("发送线程即将退出.\n");  
                        return;
                    }
                    
                    // 管道有数据（工作线程通知）
                    char cc;
                    read(m_sendpipe[0], &cc, 1);
                    
                    // 取出管道数据
                    vector<shared_ptr<st_recvmesg>> tmp_msgs;
                    {
                        lock_guard<mutex> lock(m_mutex_sq);
                        while (!m_sq.empty()) {
                            tmp_msgs.push_back(m_sq.front());
                            m_sq.pop();
                        }
                    }
                    
                    // 把报文内容保存到对应客户端的发送缓冲区
                    for (auto &ptr : tmp_msgs) {
                        {
                            lock_guard<mutex> lock(m_mutex_clientmap);
                            auto it = clientmap.find(ptr->sock);
                            if (it != clientmap.end()) {
                                it->second.sendbuffer.append(ptr->message);
                                
                                // 注册写事件
                                ev.data.fd = ptr->sock;
                                ev.events = EPOLLOUT;
                                epoll_ctl(epollfd, EPOLL_CTL_ADD, ptr->sock, &ev);
                            }
                        }
                    }
                    continue;
                }
                
                // 判断客户端的socket是否有写事件
                if (evs[ii].events & EPOLLOUT) {
                    string senddata;
                    {
                        lock_guard<mutex> lock(m_mutex_clientmap);
                        auto it = clientmap.find(evs[ii].data.fd);
                        if (it != clientmap.end()) {
                            senddata = it->second.sendbuffer;
                        }
                    }
                    
                    if (!senddata.empty()) {
                        int writen = send(evs[ii].data.fd, senddata.data(), senddata.length(), 0);
                        if (writen > 0) {
                            logfile.write("发送线程:向%d发送了%d字节.\n", evs[ii].data.fd, writen);
                            
                            lock_guard<mutex> lock(m_mutex_clientmap);
                            auto it = clientmap.find(evs[ii].data.fd);
                            if (it != clientmap.end()) {
                                it->second.sendbuffer.erase(0, writen);
                                
                                if (it->second.sendbuffer.empty()) {
                                    epoll_ctl(epollfd, EPOLL_CTL_DEL, evs[ii].data.fd, NULL);
                                    // 重新注册读事件
                                    ev.data.fd = evs[ii].data.fd;
                                    ev.events = EPOLLIN;
                                    epoll_ctl(epollfd, EPOLL_CTL_ADD, evs[ii].data.fd, &ev);
                                }
                            }
                        } else if (writen <= 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                            logfile.write("发送线程:向%d发送失败，关闭连接\n", evs[ii].data.fd);
                            epoll_ctl(epollfd, EPOLL_CTL_DEL, evs[ii].data.fd, NULL);
                            close(evs[ii].data.fd);
                            
                            lock_guard<mutex> lock(m_mutex_clientmap);
                            clientmap.erase(evs[ii].data.fd);
                        }
                    }
                }
            }
        }
        
        close(epollfd);
        logfile.write("发送线程退出\n");
    }
};

AA aa;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("\n");
        printf("Using :./webserver logfile port\n\n");
        printf("Sample:./webserver /A/log/idc/webserver.log 5088\n\n");
        printf("基于HTTP协议的数据访问接口模块。\n");
        printf("logfile 本程序运行的日志文件。\n");
        printf("port    服务端口，例如：80、8080。\n\n");
        return -1;
    }
    
    if (logfile.open(argv[1]) == false) {
        cout << "logfile.open failed.\n";
        return -1;
    }
    
    signal(2, EXIT);
    signal(15, EXIT);

    int port = atoi(argv[2]);

    // 创建线程
    thread t1(&AA::recvfunc, &aa, port);      // 接收线程
    thread t2(&AA::workfunc, &aa, 1);          // 工作线程1
    thread t3(&AA::workfunc, &aa, 2);          // 工作线程2
    thread t4(&AA::workfunc, &aa, 3);          // 工作线程3
    thread t5(&AA::sendfunc, &aa);              // 发送线程
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    
    return 0;
}

int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int initserver(const int port) {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd <= 0) {
        perror("socket failed");
        return -1;
    }
    
    // 设置端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(listenfd);
        return -1;
    }
    
    if (listen(listenfd, 100) < 0) {  // 增大监听队列
        perror("listen failed");
        close(listenfd);
        return -1;
    }
    
    return listenfd;
}

void EXIT(int sig) {
    logfile.write("程序退出, sig=%d\n", sig);
    char c = '1';
    write(aa.m_recvpipe[1], &c, 1);
    sleep(2);
    exit(0);
}

bool getvalue(const string &strget, const string &name, string &value, const int len) {
    string pattern = name + "=";
    size_t startp = strget.find(pattern);
    if (startp == string::npos) return false;
    
    startp += pattern.length();
    
    size_t endp = strget.find('&', startp);
    if (endp == string::npos) {
        endp = strget.find(' ', startp);
        if (endp == string::npos) {
            endp = strget.find("\r\n", startp);
        }
    }
    
    if (endp == string::npos) {
        value = strget.substr(startp);
    } else {
        value = strget.substr(startp, endp - startp);
    }
    
    // 限制长度
    if (len > 0 && value.length() > (size_t)len) {
        value.resize(len);
    }
    
    return true;
}

void workmain(connection &conn, const string &recvbuff, string &sendbuff, const string &clientip) {
    string username, passwd, intername;

    // 检查是否是favicon.ico请求
    if (recvbuff.find("GET /favicon.ico") != string::npos) {
        logfile.write("收到favicon.ico请求，返回空响应\n");
        sendbuff = "";  // 返回空内容
        return;
    }
    
    // 检查是否是静态文件请求
    if (recvbuff.find("GET /") != string::npos) {
        size_t pos1 = recvbuff.find("GET /") + 5;
        size_t pos2 = recvbuff.find(" ", pos1);
        string path = recvbuff.substr(pos1, pos2 - pos1);
        
        // 如果不是API请求（没有?），可能是静态文件
        if (path.find("?") == string::npos && path != "/") {
            logfile.write("静态文件请求: %s\n", path.c_str());
            sendbuff = "404 Not Found";
            return;
        }
    }

    getvalue(recvbuff, "username", username, 100);    // 解析用户名。
    getvalue(recvbuff, "passwd", passwd, 100);        // 解析密码。
    getvalue(recvbuff, "intername", intername, 100);  // 解析接口名。

    logfile.write("workmain: username=%s, passwd=%s, intername=%s, clientip=%s\n", 
                  username.c_str(), passwd.c_str(), intername.c_str(), clientip.c_str());

    // 检查参数完整性
    if (username.empty() || passwd.empty() || intername.empty()) {
        logfile.write("参数不完整\n");
        sendbuff = "<retcode>-1</retcode><message>参数不完整。</message>";
        return;
    }

    // 1）验证用户名和密码是否正确。
    sqlstatement stmt(&conn);
    stmt.prepare("select ip from T_USERINFO where username=:1 and passwd=:2 and rsts=1");
    stmt.bindin(1, username);
    stmt.bindin(2, passwd);
    string ip;
    stmt.bindout(1, ip, 1000);  // 增大缓冲区
    
    if (stmt.execute() != 0) {
        logfile.write("execute failed: %s\n", stmt.message());
        sendbuff = "<retcode>-1</retcode><message>数据库查询失败。</message>";
        return;
    }  
    if (stmt.next() != 0) {
        // 用户存在，继续验证IP
        logfile.write("用户名或密码错误: username=%s,pswd=%s\n", username.c_str(),passwd.c_str());
        sendbuff = "<retcode>-1</retcode><message>用户名或密码不正确。</message>";
        return;
    }
    
    // 2）判断客户连上来的地址是否在绑定ip地址的列表中。
    if (!ip.empty()) {
    logfile.write("原始ip='%s', 长度=%d\n", ip.c_str(), ip.length());
    logfile.write("原始clientip='%s', 长度=%d\n", clientip.c_str(), clientip.length());
    
    // 1. 彻底清理ip字符串
    string clean_ip = ip;
    
    // 去除所有空白字符（空格、制表符、换行、回车）
    clean_ip.erase(remove_if(clean_ip.begin(), clean_ip.end(), 
                  [](unsigned char c) { return isspace(c); }), clean_ip.end());
    
    // 去除所有不可见字符（ASCII码小于32的字符）
    clean_ip.erase(remove_if(clean_ip.begin(), clean_ip.end(),
                  [](unsigned char c) { return c < 32; }), clean_ip.end());
    
    logfile.write("清理后ip='%s', 长度=%d\n", clean_ip.c_str(), clean_ip.length());
    
    // 2. 清理clientip（虽然看起来正常，也清理一下）
    string clean_clientip = clientip;
    clean_clientip.erase(remove_if(clean_clientip.begin(), clean_clientip.end(),
                        [](unsigned char c) { return isspace(c) || c < 32; }), clean_clientip.end());
    
    logfile.write("清理后clientip='%s', 长度=%d\n", clean_clientip.c_str(), clean_clientip.length());
    
    
    // 4. 比较清理后的字符串
    if (clean_ip == clean_clientip) {
        logfile.write("客户连上来的地址(%s)在绑定IP列表中\n", clientip.c_str());
    } else {
        logfile.write("客户连上来的地址(%s)没有绑定在ip地址的列表中\n", clientip.c_str());
        logfile.write("请检查数据库中的IP格式，应该为纯IP地址，无空格和换行\n");
        sendbuff = "<retcode>-1</retcode><message>用户地址没有绑定在ip地址集合。</message>";
        return;
    }
}
    // 3）判断用户是否有访问接口的权限。
    stmt.prepare("select count(*) from T_USERANDINTER "
                 "where username=:1 and intername=:2 and intername in "
                 "(select intername from T_INTERCFG where rsts=1)");
    stmt.bindin(1, username);
    stmt.bindin(2, intername);
    int icount = 0;
    stmt.bindout(1, icount);
    
    if (stmt.execute() != 0) {
        logfile.write("权限查询失败: %s\n", stmt.message());
        sendbuff = "<retcode>-1</retcode><message>权限查询失败。</message>";
        return;
    }
    
    stmt.next();
    if (icount == 0) {
        sendbuff = "<retcode>-1</retcode><message>用户无权限，或接口不存在。</message>";
        return;
    }
    
    // 4）根据接口名，获取接口的配置参数。
    // 从接口参数配置表T_INTERCFG中加载接口参数。
    string selectsql, colstr, bindin;
    stmt.prepare("select selectsql, colstr, bindin from T_INTERCFG where intername=:1");
    stmt.bindin(1, intername);
    stmt.bindout(1, selectsql, 4000);
    stmt.bindout(2, colstr, 300);
    stmt.bindout(3, bindin, 300);
    
    if (stmt.execute() != 0) {
        logfile.write("接口配置查询失败: %s\n", stmt.message());
        sendbuff = "<retcode>-1</retcode><message>接口配置查询失败。</message>";
        return;
    }
    
    if (stmt.next() != 0) {
        sendbuff = "<retcode>-1</retcode><message>接口不存在。</message>";
        return;
    }

    // 5）准备查询数据的SQL语句。
    stmt.prepare(selectsql);

        //////////////////////////////////////////////////
        // 根据接口配置中的参数列表（bindin字段），从请求报文中解析出参数的值，绑定到查询数据的SQL语句中。
        // 拆分输入参数bindin。
    ccmdstr cmdstr;
    cmdstr.splittocmd(bindin,",");

        // 声明用于存放输入参数的数组。
    vector<string> invalue;
    invalue.resize(cmdstr.size());

        // 从http的GET请求报文中解析出输入参数，绑定到sql中。
    for (int ii=0;ii<cmdstr.size();ii++)
    {
        getvalue(recvbuff,cmdstr[ii].c_str(),invalue[ii],100);
        stmt.bindin(ii+1,invalue[ii]);
    }
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        // 绑定查询数据的SQL语句的输出变量。
        // 拆分colstr，可以得到结果集的字段数。
    cmdstr.splittocmd(colstr,",");

        // 用于存放结果集的数组。
    vector<string> colvalue;
    colvalue.resize(cmdstr.size());

        // 把结果集绑定到colvalue数组。
    for (int ii=0;ii<cmdstr.size();ii++)
        stmt.bindout(ii+1,colvalue[ii]);
        //////////////////////////////////////////////////

    if (stmt.execute() != 0)
    {
        logfile.write("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); 
        sformat(sendbuff,"<retcode>%d</retcode><message>%s</message>\n",stmt.rc(),stmt.message());
        return;
    }

    sendbuff="<retcode>0</retcode><message>ok</message>\n";

    sendbuff=sendbuff+"<data>\n";           // xml内容开始的标签<data>。

        //////////////////////////////////////////////////
        // 获取结果集，每获取一条记录，拼接xml。
    while (true)
    {
        if (stmt.next() != 0) break;            // 从结果集中取一条记录。

            // 拼接每个字段的xml。
        for (int ii=0;ii<cmdstr.size();ii++)
            sendbuff=sendbuff+sformat("<%s>%s</%s>",cmdstr[ii].c_str(),colvalue[ii].c_str(),cmdstr[ii].c_str());
            sendbuff=sendbuff+"<endl/>\n";    // 每行结束的标志。
    }
    
}