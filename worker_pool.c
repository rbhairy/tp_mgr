#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

#include "worker.h"
#include "worker_pool.h"

#define cont_of(ptr, type, member) \
    (ptr ? ((type*) (((char*) ptr) - offsetof(type, member))) : NULL)

static void
add_task_to_runq(worker_pool_t *wpool, task_t *task)
{
        pthread_mutex_lock(&wpool->mutex);
        {
                /* put this task into the run queue */
                queue_push(&wpool->runq, &task->qitem);

                /* tell workers about arrival of new task */
                pthread_cond_broadcast (&wpool->cond);
        }
        pthread_mutex_unlock(&wpool->mutex);
}

int
orderq(queue_item_t *qitem1, queue_item_t *qitem2)
{
        task_t *task1 = NULL;
        task_t *task2 = NULL;

        task1 = cont_of(qitem1, task_t, qitem);
        task2 = cont_of(qitem2, task_t, qitem);

        return (task1->priority > task2->priority);
}
task_t *
pick_task_from_runq(worker_pool_t *wpool)
{
        task_t *task = NULL;

        pthread_mutex_lock(&wpool->mutex);
        for (;;) {
                if (!queue_empty(&wpool->runq)) {
                        queue_item_t *item;
                        item = queue_pop(&wpool->runq);
                        task = cont_of(item, task_t, qitem);
                        break;
                }
                pthread_cond_wait(&wpool->cond, &wpool->mutex);
        }
        pthread_mutex_unlock(&wpool->mutex);

        return task;
}

static void *
poller_thread_fn(void *data)
{
        worker_pool_t *wpool = (worker_pool_t *)data;
        int rc;

        while (1) {
                /*  Wait for timeouts events. */
                rc = task_timer_wait(&wpool->task_timerset, 500);
                while (1) {
                        task_t *task;

                        rc = task_timer_events(&wpool->task_timerset, (void**)&task);
                        if (rc == -EAGAIN)
                                break;

                        add_task_to_runq(wpool, task);
                }
        }
        return NULL;
}

int
worker_pool_init(worker_pool_t *wpool, int num_workers)
{
        int ret = 0;
        int i;
        char worker_name[WORKER_NAME_LEN];

        INIT_LIST_HEAD (&wpool->task_list);
        wpool->num_workers = num_workers;
        wpool->next_task_id = 1;

        pthread_mutex_init (&wpool->mutex, NULL);
        pthread_cond_init (&wpool->cond, NULL);

        queue_init(&wpool->runq);
        queue_init(&wpool->pauseq);
        task_timerset_init(&wpool->task_timerset);

        for (i = 0; i < wpool->num_workers; i++) {
                snprintf(worker_name, WORKER_NAME_LEN, "worker-%d", i);
                ret = worker_init(&wpool->workers[i], worker_name, wpool);
                //TODO: error handling in case of partial no of worker init
        }

        /* run poller for timeout events  */
        ret = pthread_create(&wpool->poller, NULL, poller_thread_fn, wpool);
        if (ret) {
                /* TODO error handling */
                return ret;
        }

        return ret;
}


void
worker_pool_term(worker_pool_t *wpool)
{
        return;
}


/*
 * This create a new task and put it in the wait queue.
 *
 * on succesfull creation of task it returns 'task_id' to caller.
 * task id can be later used to stop/pause/resume operation.
 *
 * in case of error, return appropiate error.
 */
int add_utask(worker_pool_t *wpool, int priority, int wtime_sec,
             task_handler_fn_t fn, void *pvt_data)
{
        task_t *task = NULL;
        int task_id;

        task = (task_t *)malloc(sizeof(task_t));
        if (!task) {
                return -ENOMEM;
        }

        pthread_mutex_lock(&wpool->mutex);
        {
                task_id = wpool->next_task_id;
                ++wpool->next_task_id;

                //task initialization
                task_init(task, task_id, priority, wtime_sec, fn, pvt_data);

#ifdef TIMER_ENABLE
                task_timer_int(&task->timer, task->wtime_sec, (void*)task);
#endif
                //add task to global list
                list_add_tail(&task->list, &wpool->task_list);

#ifdef TIMER_ENABLE
                //start the timer for task
                task_timer_add(&wpool->task_timerset, &task->timer);
#else
                //queue_push(&wpool->runq, &task->qitem);
                queue_push_order(&wpool->runq, &task->qitem, orderq);
                pthread_cond_broadcast (&wpool->cond);
#endif
        }
        pthread_mutex_unlock(&wpool->mutex);

        return task_id;
}

static task_t *
get_task_from_id(worker_pool_t *wpool, int task_id)
{
        task_t *task = NULL;

        pthread_mutex_lock(&wpool->mutex);
        if (!list_empty(&wpool->task_list)) {
                list_for_each_entry(task, &wpool->task_list, list) {
                        if (task->task_id == task_id)
                                break;
                }
        }
        pthread_mutex_unlock(&wpool->mutex);
        return task;
}

int
pause_utask(worker_pool_t *wpool, int task_id)
{
        task_t *task = NULL;

        task = get_task_from_id(wpool, task_id);
        if (!task) {
                fprintf(stdout, "Invalid task ID\n");
                return -1;
        }

        if (task_get_state(task) == TASK_STATE_RUNNING) {
                worker_t *worker = task_get_cur_worker(task);
                worker_pause(worker);
        } else {
                fprintf(stdout, "task is not running\n");
                return -1;
        }

        return 0;
}

int
stop_utask(worker_pool_t *wpool, int task_id)
{
        task_t *task = NULL;

        task = get_task_from_id(wpool, task_id);
        if (!task) {
                fprintf(stdout, "Invalid task ID\n");
                return -1;
        }

        if (task_get_state(task) == TASK_STATE_RUNNING) {
                worker_t *worker = task_get_cur_worker(task);
                worker_stop(worker);
        } else {
                fprintf(stdout, "task is not running\n");
                return -1;
        }

        return 0;
}

int
resume_utask(worker_pool_t *wpool, int task_id)
{
        task_t *task = NULL;

        task = get_task_from_id(wpool, task_id);
        if (!task) {
                fprintf(stdout, "Invalid task ID\n");
                return -1;
        }

        if (task_get_state(task) == TASK_STATE_PAUSED) {
                add_task_to_runq(wpool, task);
        } else {
                fprintf(stdout, "Only pause task can be resumed.\n");
                return -1;
        }

        return 0;
}

void
yield_utask(void)
{
        return worker_should_yield();
}

void
print_utask_list(worker_pool_t *wpool)
{
        task_t *task = NULL;

        fprintf(stdout, "\t\nTask List\n");
        fprintf(stdout, "--   --------   -----\n");
        fprintf(stdout, "id   priority   state\n");
        fprintf(stdout, "--   --------   -----\n");

        pthread_mutex_lock(&wpool->mutex);
        if (!list_empty(&wpool->task_list)) {
                list_for_each_entry(task, &wpool->task_list, list) {
                        if (task->state == TASK_STATE_RUNNING) {
                                fprintf(stdout,
                                "%d    %d          %s [%s]\n",
                                task->task_id,
                                task->priority,
                                get_task_state_str(task->state),
                                ((worker_t*)task_get_cur_worker(task))->worker_name);
                        } else {
                                fprintf(stdout,
                                "%d    %d          %s\n",
                                task->task_id,
                                task->priority,
                                get_task_state_str(task->state));
                        }
                }
        }

        pthread_mutex_unlock(&wpool->mutex);
        fprintf(stdout, "-----------------------\n");
}
