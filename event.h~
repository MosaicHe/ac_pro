
#ifndef __EVENT_H
#define __EVENT_H
typedef struct event_s{
    int fd;
    void (*call_back)(int fd, int events, void *arg);  
    int events;  
    void *arg;  
    int status; 		// 1: in epoll wait list, 0 not in  
    char *p_buff; 		// recv data buffer  
    int len;
	int s_offset;  
    long last_active; 	// last active time  
}acEvent_s;

void EventSet(acEvent_s *ev, int fd, void (*call_back)(int, int, void*), void *arg);
void EventAdd(int epollFd, int events, acEvent_s *ev);
void EventDel(int epollFd, acEvent_s *ev);

#endif
