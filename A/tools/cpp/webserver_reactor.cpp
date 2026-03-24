#include "mpublic.h"
#include "_ooci.h"
#include "/A/tools/cpp/reactor.h"   
using namespace idc;

// 全局前向声明
clogfile logfile;
void EXIT(int sig);
void workmain(connection &conn, const string &recvbuff, string &sendbuff, const string &clientip);

// 结构体定义
struct st_client {
    string clientip;
    int clientatime;
    string recvbuffer;
    string sendbuffer;
};

struct st_recvmesg {
    int sock = 0;
    string clientip;
    string message;
    st_recvmesg(int m_sock, const string &m_clientip, const string &m_message)
        : sock(m_sock), clientip(m_clientip), message(m_message) {}
};

// 前向声明类
class AA;
class ClientChannel;

// =============================================================================
// ClientChannel 类声明（先声明，实现放在 AA 定义之后）
// =============================================================================
class ClientChannel : public Channel {
public:
    ClientChannel(int fd, const string& clientip, EventLoop* loop, AA* aa);
    ~ClientChannel();

    void handle_read();
    void handle_write();
    void handle_close();
    void handle_error();

    void append_sendbuffer(const string& data);
    void enable_read();
    string clientip() const { return m_clientip; }
    int last_active() const { return m_last_active; }

private:
    string m_clientip;
    string m_recvbuffer;
    string m_sendbuffer;
    int m_last_active;
    EventLoop* m_loop;
    AA* m_aa;
};

// =============================================================================
// AA 类定义
// =============================================================================
class AA {
public:
    AA();
    ~AA();

    void inrq(int sock, const string &clientip, const string &message);
    void insq(int sock, const string &message);

    void recvfunc(const int listenport);
    void workfunc(int id);
    void sendfunc();

    void register_client(int fd, ClientChannel* channel);
    void unregister_client(int fd);
    void stop();

    int m_recvpipe[2] = {0};   // 用于主线程通知接收线程退出

private:
    queue<shared_ptr<st_recvmesg>> m_rq;
    mutex m_mutex_rq;
    condition_variable m_cond_rq;

    queue<shared_ptr<st_recvmesg>> m_sq;
    mutex m_mutex_sq;
    int m_sendpipe[2] = {0};

    unordered_map<int, ClientChannel*> m_client_channels;
    mutex m_mutex_clientmap;

    atomic_bool m_exit;

    EventLoop m_recv_loop;
    EventLoop m_send_loop;

    int initserver(const int port);
};

// =============================================================================
// AcceptChannel 类定义
// =============================================================================
class AcceptChannel : public Channel {
public:
    AcceptChannel(int fd, EventLoop* loop, AA* aa)
        : Channel(fd), m_loop(loop), m_aa(aa) {
        set_events(EPOLLIN);
        set_read_callback(std::bind(&AcceptChannel::handle_accept, this));
    }

    void handle_accept() {
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        int clientfd = accept(m_fd, (struct sockaddr*)&clientaddr, &len);
        if (clientfd < 0) return;

        string clientip = inet_ntoa(clientaddr.sin_addr);
        logfile.write("接受新连接:client(socket=%d, ip=%s).\n", clientfd, clientip.c_str());

        auto* client_channel = new ClientChannel(clientfd, clientip, m_loop, m_aa);
        client_channel->set_events(EPOLLIN);
        m_loop->update_channel(client_channel);
        m_aa->register_client(clientfd, client_channel);
    }

private:
    EventLoop* m_loop;
    AA* m_aa;
};

// =============================================================================
// ClientChannel 成员函数实现
// =============================================================================
ClientChannel::ClientChannel(int fd, const string& clientip, EventLoop* loop, AA* aa)
    : Channel(fd), m_clientip(clientip), m_loop(loop), m_aa(aa) {
    set_read_callback(std::bind(&ClientChannel::handle_read, this));
    set_write_callback(std::bind(&ClientChannel::handle_write, this));
    set_close_callback(std::bind(&ClientChannel::handle_close, this));
    set_error_callback(std::bind(&ClientChannel::handle_error, this));
    m_last_active = time(0);
}

ClientChannel::~ClientChannel() {
    logfile.write("客户端断开连接,socket=%d, ip=%s\n", m_fd, m_clientip.c_str());
    m_aa->unregister_client(m_fd);
}

void ClientChannel::handle_read() {
    char buff[1024];
    int bufflen = recv(m_fd, buff, sizeof(buff), 0);
    if (bufflen <= 0) {
        handle_close();
        return;
    }

    logfile.write("接收线程:recv %d,%d bytes\n", m_fd, bufflen);
    m_recvbuffer.append(buff, bufflen);
    m_last_active = time(0);

    // 检查是否收到完整的HTTP请求（以\r\n\r\n结尾）
    if (m_recvbuffer.length() >= 4 &&
        m_recvbuffer.substr(m_recvbuffer.length() - 4) == "\r\n\r\n") {
        logfile.write("接收到了一个完整的请求报文。\n");
        m_aa->inrq(m_fd, m_clientip, m_recvbuffer);
        m_recvbuffer.clear();

        // 暂停读事件，等待业务处理完成
        set_events(0);
        m_loop->update_channel(this);
    }
}

void ClientChannel::handle_write() {
    if (m_sendbuffer.empty()) {
        set_events(EPOLLIN);
        m_loop->update_channel(this);
        return;
    }

    int writen = send(m_fd, m_sendbuffer.data(), m_sendbuffer.length(), 0);
    if (writen > 0) {
        logfile.write("发送线程:向%d发送了%d字节.\n", m_fd, writen);
        m_sendbuffer.erase(0, writen);
        if (m_sendbuffer.empty()) {
            set_events(EPOLLIN);
            m_loop->update_channel(this);
        }
    } else if (writen <= 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        handle_error();
    }
}

void ClientChannel::handle_close() {
    m_loop->remove_channel(this);
    delete this;
}

void ClientChannel::handle_error() {
    logfile.write("socket=%d 发生错误\n", m_fd);
    handle_close();
}

void ClientChannel::append_sendbuffer(const string& data) {
    m_sendbuffer.append(data);
    set_events(EPOLLIN | EPOLLOUT);
    m_loop->update_channel(this);
}

void ClientChannel::enable_read() {
    set_events(EPOLLIN);
    m_loop->update_channel(this);
}

// =============================================================================
// AA 成员函数实现
// =============================================================================
AA::AA() {
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

AA::~AA() {
    if (m_sendpipe[0] > 0) close(m_sendpipe[0]);
    if (m_sendpipe[1] > 0) close(m_sendpipe[1]);
    if (m_recvpipe[0] > 0) close(m_recvpipe[0]);
    if (m_recvpipe[1] > 0) close(m_recvpipe[1]);
}

void AA::inrq(int sock, const string &clientip, const string &message) {
    shared_ptr<st_recvmesg> ptr = make_shared<st_recvmesg>(sock, clientip, message);
    {
        lock_guard<mutex> lock(m_mutex_rq);
        m_rq.push(ptr);
    }
    m_cond_rq.notify_one();
}

void AA::insq(int sock, const string &message) {
    {
        lock_guard<mutex> lock(m_mutex_sq);
        shared_ptr<st_recvmesg> ptr = make_shared<st_recvmesg>(sock, "", message);
        m_sq.push(ptr);
    }
    char c = '1';
    write(m_sendpipe[1], &c, 1);
}

void AA::recvfunc(const int listenport) {
    int listenfd = initserver(listenport);
    if (listenfd < 0) {
        logfile.write("初始化服务器失败\n");
        return;
    }

    AcceptChannel* accept_channel = new AcceptChannel(listenfd, &m_recv_loop, this);
    m_recv_loop.update_channel(accept_channel);

    // 接收线程退出管道
    class PipeChannel : public Channel {
    public:
        PipeChannel(int fd, EventLoop* loop, AA* aa) : Channel(fd), m_loop(loop), m_aa(aa) {
            set_events(EPOLLIN);
            set_read_callback(std::bind(&PipeChannel::handle_read, this));
        }
        void handle_read() {
            char cc;
            read(m_fd, &cc, 1);
            logfile.write("程序即将退出.\n");
            m_aa->stop();
            m_loop->stop();
        }
    private:
        EventLoop* m_loop;
        AA* m_aa;
    };

    PipeChannel* pipe_channel = new PipeChannel(m_recvpipe[0], &m_recv_loop, this);
    m_recv_loop.update_channel(pipe_channel);

    logfile.write("接收线程启动，监听端口：%d\n", listenport);
    m_recv_loop.loop(1000);
    logfile.write("接收线程退出\n");
}

void AA::workfunc(int id) {
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

        string sendbuff;
        workmain(conn, ptr->message, sendbuff, ptr->clientip);

        string http_response =
            "HTTP/1.1 200 OK\r\n"
            "Server: webserver\r\n"
            "Content-Type: text/html;charset=utf-8\r\n"
            "Content-Length: " + to_string(sendbuff.size()) + "\r\n"
            "\r\n" + sendbuff;

        insq(ptr->sock, http_response);

        {
            lock_guard<mutex> lock(m_mutex_clientmap);
            auto it = m_client_channels.find(ptr->sock);
            if (it != m_client_channels.end()) {
                it->second->enable_read();
            }
        }
    }
}

void AA::sendfunc() {
    class SendPipeChannel : public Channel {
    public:
        SendPipeChannel(int fd, EventLoop* loop, AA* aa) : Channel(fd), m_loop(loop), m_aa(aa) {
            set_events(EPOLLIN);
            set_read_callback(std::bind(&SendPipeChannel::handle_read, this));
        }

        void handle_read() {
            char cc;
            read(m_fd, &cc, 1);

            vector<shared_ptr<st_recvmesg>> tmp_msgs;
            {
                lock_guard<mutex> lock(m_aa->m_mutex_sq);
                while (!m_aa->m_sq.empty()) {
                    tmp_msgs.push_back(m_aa->m_sq.front());
                    m_aa->m_sq.pop();
                }
            }

            for (auto &ptr : tmp_msgs) {
                lock_guard<mutex> lock(m_aa->m_mutex_clientmap);
                auto it = m_aa->m_client_channels.find(ptr->sock);
                if (it != m_aa->m_client_channels.end()) {
                    it->second->append_sendbuffer(ptr->message);
                }
            }
        }

    private:
        EventLoop* m_loop;
        AA* m_aa;
    };

    SendPipeChannel* pipe_channel = new SendPipeChannel(m_sendpipe[0], &m_send_loop, this);
    m_send_loop.update_channel(pipe_channel);

    logfile.write("发送线程启动\n");
    m_send_loop.loop(1000);
    logfile.write("发送线程退出\n");
}

void AA::register_client(int fd, ClientChannel* channel) {
    lock_guard<mutex> lock(m_mutex_clientmap);
    m_client_channels[fd] = channel;
}

void AA::unregister_client(int fd) {
    lock_guard<mutex> lock(m_mutex_clientmap);
    m_client_channels.erase(fd);
}

void AA::stop() {
    m_exit = true;
    m_cond_rq.notify_all();
    m_recv_loop.stop();
    m_send_loop.stop();
}

int AA::initserver(const int port) {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd <= 0) {
        perror("socket failed");
        return -1;
    }

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

    if (listen(listenfd, 100) < 0) {
        perror("listen failed");
        close(listenfd);
        return -1;
    }

    return listenfd;
}

// =============================================================================
// 全局对象和辅助函数
// =============================================================================
AA aa;

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
    if (len > 0 && value.length() > (size_t)len) value.resize(len);
    return true;
}

void workmain(connection &conn, const string &recvbuff, string &sendbuff, const string &clientip) {
    string username, passwd, intername;

    if (recvbuff.find("GET /favicon.ico") != string::npos) {
        logfile.write("收到favicon.ico请求，返回空响应\n");
        sendbuff = "";
        return;
    }

    if (recvbuff.find("GET /") != string::npos) {
        size_t pos1 = recvbuff.find("GET /") + 5;
        size_t pos2 = recvbuff.find(" ", pos1);
        string path = recvbuff.substr(pos1, pos2 - pos1);
        if (path.find("?") == string::npos && path != "/") {
            logfile.write("静态文件请求: %s\n", path.c_str());
            sendbuff = "404 Not Found";
            return;
        }
    }

    getvalue(recvbuff, "username", username, 100);
    getvalue(recvbuff, "passwd", passwd, 100);
    getvalue(recvbuff, "intername", intername, 100);

    logfile.write("workmain: username=%s, passwd=%s, intername=%s, clientip=%s\n",
                  username.c_str(), passwd.c_str(), intername.c_str(), clientip.c_str());

    if (username.empty() || passwd.empty() || intername.empty()) {
        logfile.write("参数不完整\n");
        sendbuff = "<retcode>-1</retcode><message>参数不完整。</message>";
        return;
    }

    // 1）验证用户名和密码
    sqlstatement stmt(&conn);
    stmt.prepare("select ip from T_USERINFO where username=:1 and passwd=:2 and rsts=1");
    stmt.bindin(1, username);
    stmt.bindin(2, passwd);
    string ip;
    stmt.bindout(1, ip, 1000);

    if (stmt.execute() != 0) {
        logfile.write("execute failed: %s\n", stmt.message());
        sendbuff = "<retcode>-1</retcode><message>数据库查询失败。</message>";
        return;
    }
    if (stmt.next() != 0) {
        logfile.write("用户名或密码错误: username=%s,pswd=%s\n", username.c_str(), passwd.c_str());
        sendbuff = "<retcode>-1</retcode><message>用户名或密码不正确。</message>";
        return;
    }

    // 2）IP绑定验证
    if (!ip.empty()) {
        string clean_ip = ip;
        clean_ip.erase(remove_if(clean_ip.begin(), clean_ip.end(),
                      [](unsigned char c) { return isspace(c); }), clean_ip.end());
        clean_ip.erase(remove_if(clean_ip.begin(), clean_ip.end(),
                      [](unsigned char c) { return c < 32; }), clean_ip.end());

        string clean_clientip = clientip;
        clean_clientip.erase(remove_if(clean_clientip.begin(), clean_clientip.end(),
                            [](unsigned char c) { return isspace(c) || c < 32; }), clean_clientip.end());

        if (clean_ip != clean_clientip) {
            logfile.write("客户连上来的地址(%s)没有绑定在ip地址的列表中\n", clientip.c_str());
            sendbuff = "<retcode>-1</retcode><message>用户地址没有绑定在ip地址集合。</message>";
            return;
        }
    }

    // 3）接口权限
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

    // 4）获取接口配置
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

    // 5）准备查询SQL
    stmt.prepare(selectsql);
    ccmdstr cmdstr;
    cmdstr.splittocmd(bindin, ",");
    vector<string> invalue(cmdstr.size());

    for (int ii = 0; ii < cmdstr.size(); ++ii) {
        getvalue(recvbuff, cmdstr[ii].c_str(), invalue[ii], 100);
        stmt.bindin(ii + 1, invalue[ii]);
    }

    cmdstr.splittocmd(colstr, ",");
    vector<string> colvalue(cmdstr.size());
    for (int ii = 0; ii < cmdstr.size(); ++ii)
        stmt.bindout(ii + 1, colvalue[ii]);

    if (stmt.execute() != 0) {
        logfile.write("stmt.execute() failed.\n%s\n%s\n", stmt.sql(), stmt.message());
        sformat(sendbuff, "<retcode>%d</retcode><message>%s</message>\n", stmt.rc(), stmt.message());
        return;
    }

    sendbuff = "<retcode>0</retcode><message>ok</message>\n<data>\n";
    while (stmt.next() == 0) {
        for (int ii = 0; ii < cmdstr.size(); ++ii)
            sendbuff += sformat("<%s>%s</%s>", cmdstr[ii].c_str(), colvalue[ii].c_str(), cmdstr[ii].c_str());
        sendbuff += "<endl/>\n";
    }
    sendbuff += "</data>";
}

// =============================================================================
// 主函数
// =============================================================================
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("\n");
        printf("Using :./webserver logfile port\n\n");
        printf("Sample:./webserver_reactor /A/log/idc/webserver_reactor.log 5008\n\n");
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

    thread t1(&AA::recvfunc, &aa, port);
    thread t2(&AA::workfunc, &aa, 1);
    thread t3(&AA::workfunc, &aa, 2);
    thread t4(&AA::workfunc, &aa, 3);
    thread t5(&AA::sendfunc, &aa);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    return 0;
}