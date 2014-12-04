#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "event.h"

#define MAX_EVENTS 500
#define PORT 8080
#define SIZE 1024


int g_epollFd; 
acEvent_s g_Events[MAX_EVENTS+1]; 		// g_Events[MAX_EVENTS] is used by listen fd  
void RecvData(int fd, int events, void *arg);  
void SendData(int fd, int events, void *arg);  

//设置socket连接为非阻塞模式
void setnonblocking(int sockfd) {
    int opts;

    opts = fcntl(sockfd, F_GETFL);
    if(opts < 0) {
        perror("fcntl(F_GETFL)\n");
        exit(1);
    }
    opts = (opts | O_NONBLOCK);
    if(fcntl(sockfd, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)\n");
        exit(1);
    }
}

// receive data  
void RecvData(int fd, int events, void *arg)  
{  
   acEvent_s *ev = (acEvent_s*)arg;  
    int len;
	ev->p_buff=malloc(SIZE);
	bzero(ev->p_buff, SIZE);

    // receive data
    len = recv(fd, ev->p_buff+ev->len, sizeof(ev->p_buff)-1-ev->len, 0);
    EventDel(g_epollFd, ev);
    if(len > 0)
    {
        ev->len += len;
        ev->p_buff[len] = '\0';  
        printf("C[%d]:%s\n", fd, ev->p_buff);  
    }
    else if(len == 0)
    {  
        close(ev->fd);  
        printf("[fd=%d] pos[%ld], closed gracefully.\n", fd, ev-g_Events);  
    }  
    else  
    {  
        close(ev->fd);  
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));  
    }  
}  
// send data  
void SendData(int fd, int events, void *arg)  
{  
   acEvent_s *ev = (acEvent_s*)arg;  
    int len;  
    // send data  
    len = send(fd, ev->p_buff + ev->s_offset, ev->len - ev->s_offset, 0);
    if(len > 0)  
    {
        printf("send[fd=%d], [%d<->%d]%s\n", fd, len, ev->len, ev->p_buff);
        ev->s_offset += len;
    }  
    else  
    {  
        close(ev->fd);  
        EventDel(g_epollFd, ev);  
        printf("send[fd=%d] error[%d]\n", fd, errno);  
    }  
}

void AcceptConn(int fd, int events, void *arg)  
{  
    struct sockaddr_in sin;  
    socklen_t len = sizeof(struct sockaddr_in);  
    int nfd, i;  
    // accept  
    if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) == -1){  
        if(errno != EAGAIN && errno != EINTR)  
        { 
        }
        printf("%s: accept, %d", __func__, errno);  
        return;
    }
    do{  
        for(i = 0; i < MAX_EVENTS; i++){  
            if(g_Events[i].status == 0)
                break;  
 
        }
        if(i == MAX_EVENTS){  
            printf("%s:max connection limit[%d].", __func__, MAX_EVENTS);  
            break;  
        }
        // set nonblocking
		setnonblocking(nfd);
        // add a read event for receive data  
        EventSet(&g_Events[i], nfd, RecvData, &g_Events[i]);  
        EventAdd(g_epollFd, EPOLLIN, &g_Events[i]);
    }while(0);
    printf("new conn[%s:%d][time:%ld], pos[%d]\n", inet_ntoa(sin.sin_addr), 
            ntohs(sin.sin_port), g_Events[i].last_active, i);  
}

int main(int args, char** argv)
{
    struct epoll_event events[MAX_EVENTS];
    int listenfd, nfds, g_epollFd, i;
    struct sockaddr_in local;
   
	g_epollFd = epoll_create(MAX_EVENTS);
    if (g_epollFd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    //create listen socket
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("sockfd\n");
        exit(1);
    }
    setnonblocking(listenfd);

	EventSet(&g_Events[MAX_EVENTS], listenfd, AcceptConn, &g_Events[MAX_EVENTS]);  
	EventAdd(g_epollFd, EPOLLIN, &g_Events[MAX_EVENTS]); 

    bzero(&local, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);;
    local.sin_port = htons(PORT);
    if( bind(listenfd, (struct sockaddr *) &local, sizeof(local)) < 0) {
        perror("bind\n");
        exit(1);
    }
    listen(listenfd, 20);

    for (;;) {
        nfds = epoll_wait(g_epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }
		// a simple timeout check here, every time 100, better to use a mini-heap, and add timer event
		int checkPos = 0; 
		long now = time(NULL);  
        for(i = 0; i < 100; i++, checkPos++) // doesn't check listen fd  
        {  
            if(checkPos == MAX_EVENTS) 
				checkPos = 0; // recycle  
            if(g_Events[checkPos].status != 1) 
				continue;  
            long duration = now - g_Events[checkPos].last_active;  
            if(duration >= 60) // 60s timeout  
            {
                close(g_Events[checkPos].fd);
                printf("[fd=%d] timeout[%ld--%ld].\n", g_Events[checkPos].fd, g_Events[checkPos].last_active, now);  
                EventDel(g_epollFd, &g_Events[checkPos]);  
            }  
        }  
        // wait for events to happen  
        int fds = epoll_wait(g_epollFd, events, MAX_EVENTS, 1000);  
        if(fds < 0){  
            printf("epoll_wait error, exit\n");  
            break;  
        }
        for(i = 0; i < fds; i++){  
            acEvent_s *ev = (acEvent_s*)events[i].data.ptr;  
            if((events[i].events & EPOLLIN) && ( ev->events & EPOLLIN)) // read event  
            {  
                ev->call_back(ev->fd, events[i].events, ev->arg);  
            }  
            if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) // write event  
            {  
                ev->call_back(ev->fd, events[i].events, ev->arg);  
            }
        }
  
    }

    return 0;
}
