#ifndef TIMER_H_
#define TIMER_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/epoll.h>

#define POLLER_MAX_EVENTS 64

typedef struct task_timer_ {
        //struct list_head list;
        void *src;
        int timeout;
        int tfd;
} task_timer_t;

typedef struct task_timerset_ {
        //struct list_head timeouts;
        int epfd;
        int nevents;
        int index;
        struct epoll_event events[POLLER_MAX_EVENTS];
} task_timerset_t;

void task_timerset_init(task_timerset_t *timerset);

int task_timer_int(task_timer_t *timer, int timeout, void *src);
int task_timer_add(task_timerset_t *timerset, task_timer_t *timer);
int task_timer_wait(task_timerset_t *timerset, int timeout);
int task_timer_events(task_timerset_t *timerset, void **src);

#endif /* TIMER_H_ */
