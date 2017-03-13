#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "worker_pool.h"
#include "worker.h"
#include "task.h"

#define UCTX_STACK_SIZE 4096

static pthread_key_t task_ctx_data_key;

void *
get_task_ctx_data()
{
        void   *task = NULL;
        task = pthread_getspecific(task_ctx_data_key);
        return task;
}

int
init_task_ctx_data ()
{
        int  ret = 0;
        ret = pthread_key_create(&task_ctx_data_key, NULL);
        return ret;
}

int
set_task_ctx_data (void *task)
{
        int ret = 0;
        ret = pthread_setspecific(task_ctx_data_key, task);
        return ret;
}

void
internal_task_fn (void *data)
{
        task_t *task = get_task_ctx_data();
        task_set_state(task, TASK_STATE_RUNNING);
        task->fn(task->pvt_data);
        task_set_state(task, TASK_STATE_COMPLETED);
        setcontext(&task->worker->ctx);
}

void worker_should_yield(void)
{
        task_t *task = get_task_ctx_data();
        worker_event_type_t event = WORKER_ETYPE_INVALID;

        if (!task || !task->worker) {
                return;
        }

        pthread_mutex_lock(&task->worker->mutex);
        event = task->worker->event;
        task->worker->event = WORKER_ETYPE_RUN;
        pthread_mutex_unlock(&task->worker->mutex);

        if (event == WORKER_ETYPE_PAUSE) {
                task_set_state(task, TASK_STATE_PAUSED);
                swapcontext(&task->ctx, &task->worker->ctx);
                task_set_state(task, TASK_STATE_RUNNING);
        } else if (event == WORKER_ETYPE_STOP) {
                task_set_state(task, TASK_STATE_STOPPED);
                setcontext(&task->worker->ctx);
        }

        return;
}

static void *
worker_thread_fn(void *data)
{
        worker_t *worker = (worker_t*)data;
        task_t *task;

        fprintf(stdout, "gc: %s running\n", worker->worker_name);
        while ((task = pick_task_from_runq(worker->wpool)))
        {
                fprintf(stdout, "gc: %s got task\n", worker->worker_name);
                task_set_cur_worker(task, worker);
                set_task_ctx_data(task);
                if (task->state == TASK_STATE_IDLE) {
                        getcontext(&task->ctx);
                        task->ctx.uc_link = 0;
                        task->ctx.uc_stack.ss_sp = malloc(UCTX_STACK_SIZE);
                        task->ctx.uc_stack.ss_size = UCTX_STACK_SIZE;
                        task->ctx.uc_stack.ss_flags = 0;
                        makecontext(&task->ctx, (void*)&internal_task_fn, 0);
                        swapcontext(&task->worker->ctx, &task->ctx);
                } else if (task_get_state(task) == TASK_STATE_PAUSED) {
                        fprintf(stdout, "PAUSE TO Running\n");
                        swapcontext(&task->worker->ctx, &task->ctx);
                } else {
                        fprintf(stderr, "[ERROR] %s: task is in invalid state\n",
                                worker->worker_name);
                }
        }
        return NULL;
}

/* worker will pause current task and will new one */
void
worker_pause(worker_t *worker)
{
        pthread_mutex_lock(&worker->mutex);
        worker->event = WORKER_ETYPE_PAUSE;
        pthread_mutex_unlock(&worker->mutex);
}

/* worker will stop current task and will new one */
void
worker_stop(worker_t *worker)
{
        pthread_mutex_lock(&worker->mutex);
        worker->event = WORKER_ETYPE_STOP;
        pthread_mutex_unlock(&worker->mutex);
}

int
worker_init(worker_t *worker, char *worker_name, void *wpool)
{
        int ret = 0;

        init_task_ctx_data();

        pthread_mutex_init (&worker->mutex, NULL);
        pthread_cond_init (&worker->cond, NULL);

        strncpy(worker->worker_name, worker_name, WORKER_NAME_LEN);
        worker->wpool = wpool;
        worker->event = WORKER_ETYPE_RUN;

        ret = pthread_create(&worker->thread, NULL, worker_thread_fn, worker);
        if (ret) {
                return ret;
        }

        return 0;
}

void
worker_term(worker_t *worker)
{
        //TODO: send the term signal and join
        return;
}
