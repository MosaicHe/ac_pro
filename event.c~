#include<stdio.h>
#include <sys/epoll.h>
#include <time.h>
#include "event.h"

// set event  
void EventSet(acEvent_s *ev, int fd, void (*call_back)(int, int, void*), void *arg)  
{  
    ev->fd = fd;  
    ev->call_back = call_back;  
    ev->events = 0;
    ev->arg	 = arg;  
    ev->status = 0;
    ev->p_buff = NULL;
    ev->s_offset = 0;  
    ev->len = 0;
    ev->last_active = time(NULL);
} 


// add/mod an event to epoll  
void EventAdd(int epollFd, int events, acEvent_s *ev)  
{  
    struct epoll_event epv = {0, {0}};  
    int op;  
    epv.data.ptr = ev;  
    epv.events = ev->events = events;  
    if(ev->status == 1){  
        op = EPOLL_CTL_MOD;  
    }  
    else{  
        op = EPOLL_CTL_ADD;  
        ev->status = 1;  
    }  
    if(epoll_ctl(epollFd, op, ev->fd, &epv) < 0)  
        printf("Event Add failed[fd=%d], evnets[%d]\n", ev->fd, events);  
    else  
        printf("Event Add OK[fd=%d], op=%d, evnets[%0X]\n", ev->fd, op, events);  
} 


// delete an event from epoll  
void EventDel(int epollFd, acEvent_s *ev)  
{  
    struct epoll_event epv = {0, {0}};  
    if(ev->status != 1) return;  
    epv.data.ptr = ev;  
    ev->status = 0;
    epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, &epv);  
} 

