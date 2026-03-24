#ifndef REACTOR_H
#define REACTOR_H

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

/**
 * Reactor核心抽象类
 */

// 工具函数：设置非阻塞
inline int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

//=============================================================================
// Channel类：封装一个文件描述符的事件和回调
//=============================================================================
class Channel {
public:
    using EventCallback = std::function<void(uint32_t events)>;
    using ReadCallback = std::function<void()>;
    using WriteCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;
    using ErrorCallback = std::function<void()>;

    Channel(int fd) : m_fd(fd), m_events(0), m_revents(0) {
        set_nonblock(m_fd);
    }

    virtual ~Channel() {
        // 不在这里close fd，由外部管理
    }

    int fd() const { return m_fd; }
    uint32_t events() const { return m_events; }
    void set_events(uint32_t events) { m_events = events; }
    void set_revents(uint32_t revents) { m_revents = revents; }

    // 设置回调
    void set_read_callback(ReadCallback cb) { m_readCallback = cb; }
    void set_write_callback(WriteCallback cb) { m_writeCallback = cb; }
    void set_close_callback(CloseCallback cb) { m_closeCallback = cb; }
    void set_error_callback(ErrorCallback cb) { m_errorCallback = cb; }

    // 处理事件
    virtual void handle_event() {
        if (m_revents & (EPOLLIN | EPOLLRDHUP)) {
            if (m_readCallback) m_readCallback();
        }
        if (m_revents & EPOLLOUT) {
            if (m_writeCallback) m_writeCallback();
        }
        if (m_revents & EPOLLERR) {
            if (m_errorCallback) m_errorCallback();
        }
        if (m_revents & EPOLLHUP) {
            if (m_closeCallback) m_closeCallback();
        }
    }

protected:
    int m_fd;                       // 文件描述符
    uint32_t m_events;               // 关注的事件
    uint32_t m_revents;              //返回的事件
    
    ReadCallback m_readCallback;     // 读事件回调
    WriteCallback m_writeCallback;   // 写事件回调
    CloseCallback m_closeCallback;   // 关闭事件回调
    ErrorCallback m_errorCallback;   // 错误事件回调
};

//=============================================================================
// EventLoop类：事件循环，封装epoll
//=============================================================================
class EventLoop {
public:
    EventLoop() : m_exit(false) {
        m_epollfd = epoll_create1(0);
        if (m_epollfd == -1) {
            perror("epoll_create1 failed");
            exit(1);
        }
    }

    ~EventLoop() {
        if (m_epollfd > 0) close(m_epollfd);
    }

    // 注册Channel到事件循环
    void update_channel(Channel* channel) {
        struct epoll_event ev;
        ev.events = channel->events();
        ev.data.ptr = channel;  // 使用ptr直接指向Channel对象
        
        auto it = m_channels.find(channel->fd());
        if (it == m_channels.end()) {
            // 新Channel，执行ADD
            if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, channel->fd(), &ev) == 0) {
                m_channels[channel->fd()] = channel;
            }
        } else {
            // 已存在的Channel，执行MOD
            epoll_ctl(m_epollfd, EPOLL_CTL_MOD, channel->fd(), &ev);
        }
    }

    // 从事件循环中移除Channel
    void remove_channel(Channel* channel) {
        auto it = m_channels.find(channel->fd());
        if (it != m_channels.end()) {
            epoll_ctl(m_epollfd, EPOLL_CTL_DEL, channel->fd(), nullptr);
            m_channels.erase(it);
        }
    }

    // 事件循环主函数
    void loop(int timeout_ms = 1000) {
        struct epoll_event evs[128];
        
        while (!m_exit) {
            int nfds = epoll_wait(m_epollfd, evs, 128, timeout_ms);
            
            if (nfds < 0) {
                if (errno != EINTR) {
                    perror("epoll_wait failed");
                }
                continue;
            }
            
            // 处理就绪的事件
            for (int i = 0; i < nfds; i++) {
                Channel* channel = static_cast<Channel*>(evs[i].data.ptr);
                if (channel) {
                    channel->set_revents(evs[i].events);
                    channel->handle_event();
                }
            }
        }
    }

    // 停止事件循环
    void stop() { m_exit = true; }

private:
    int m_epollfd;
    std::unordered_map<int, Channel*> m_channels;
    std::atomic<bool> m_exit;
};

#endif // REACTOR_H