#pragma once

struct epoll_event;

class EpollManager {
public:
    bool Init();
    int GetEvents(struct epoll_event *events, int max_events);
    bool AddFd(int fd, void *user_data);
    void RemoveFd(int fd);

private:
    int m_EpollFd;
};

