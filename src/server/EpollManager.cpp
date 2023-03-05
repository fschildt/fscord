#include <EpollManager.h>

#include <sys/epoll.h>
#include <stdio.h>


bool EpollManager::Init() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return false;
    }

    m_EpollFd = epoll_fd;

    return true;
}

int EpollManager::GetEvents(struct epoll_event *events, int max_events) {
    int cnt_events = epoll_wait(m_EpollFd, events, max_events, -1);
    if (cnt_events == -1) {
        perror("epoll_wait:");
    }

    return cnt_events;
}

bool EpollManager::AddFd(int fd, void *user_data) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = user_data;

    if (epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl [EPOLL_CTL_ADD]");
        return false;
    }
    return true;
}

void EpollManager::RemoveFd(int fd) {
    int deleted = epoll_ctl(m_EpollFd, EPOLL_CTL_DEL, fd, 0); 
    if (deleted == -1) {
        perror("epoll_ctl [EPOLL_CTL_DEL]");
    }
}


