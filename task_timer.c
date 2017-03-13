#include <stdlib.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

#include "task_timer.h"

void task_timerset_init(task_timerset_t *timerset)
{
        //INIT_LIST_HEAD(&timerset->timeouts);
        timerset->epfd = epoll_create(1);
}

void task_timerset_term(task_timerset_t *timerset)
{
        //TODO:
        //INIT_LIST_HEAD(&timerset->timeouts);
        close(timerset->epfd);
}

int
task_timer_int(task_timer_t *timer, int timeout, void *src)
{
        struct itimerspec ts;


        timer->timeout = timeout;
        timer->src = src;

        timer->tfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timer->tfd == -1)
                return -1;

        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;
        ts.it_value.tv_sec = timer->timeout;
        ts.it_value.tv_nsec = 0;

        if ((timerfd_settime(timer->tfd, 0, &ts, NULL) < 0)) {
                close(timer->tfd);
                return -1;
        }

        return 0;
}

int
task_timer_add(task_timerset_t *timerset, task_timer_t *timer)
{
        struct epoll_event ev;

        ev.events = EPOLLIN;
        ev.data.ptr = timer;

        //list_add_tail (&timerset->timeouts, &timer->list);
        if ((epoll_ctl(timerset->epfd, EPOLL_CTL_ADD, timer->tfd, &ev)) < 0)
                return -1;
        return 0;
}

int
task_timer_rm(task_timerset_t *timerset, task_timer_t *timer)
{
        epoll_ctl(timerset->epfd, EPOLL_CTL_DEL, timer->tfd, NULL);
        close(timerset->epfd);
        return 0;
}

int task_timer_wait(task_timerset_t *timerset, int timeout)
{
        int nevents;

        /*  Clear all existing events. */
        timerset->nevents = 0;
        timerset->index = 0;

        /*  Wait for new events. */
        while (1) {
                nevents = epoll_wait(timerset->epfd, timerset->events,
                                     POLLER_MAX_EVENTS, timeout);
                if ((nevents == -1) && (errno == EINTR))
                        continue;

                break;
        }

        timerset->nevents = nevents;
        return 0;
}

int task_timer_events(task_timerset_t *timerset, void **src)
{
        int index = 0;
        task_timer_t *timer;

        /* If there is no stored event, let the caller know. */
        if (timerset->index >= timerset->nevents)
                return -EAGAIN;

        /* Skip over empty events. */
        while (timerset->index < timerset->nevents) {
                if (timerset->events[timerset->index].events != 0)
                        break;
                ++timerset->index;
        }

        index = timerset->index;
        ++timerset->index;

        /* Return next event to the caller. Remove the event from the set. */
        if ((timerset->events[index].events & EPOLLIN)) {
                uint64_t res = 0;
                timer = (task_timer_t *)timerset->events[index].data.ptr;
                read(timer->tfd, &res, sizeof(res));
                *src = timer->src;
                return 0;
        } else {
                return -1;
        }
}
