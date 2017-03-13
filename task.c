#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "task.h"
#include "task_timer.h"

static char *task_state_to_string[] = {
        "IDLE     ",
        "RUNNING  ",
        "PAUSED   ",
        "STOPPED  ",
        "COMPLETED"
};

char *
get_task_state_str(task_state_t task_state)
{
        return task_state_to_string[task_state];
}

void
task_set_state(task_t *task, task_state_t new_state)
{
        task_state_t old_state = task->state;

        if (old_state == new_state)
                return;

        task->state = new_state;
}

task_state_t
task_get_state(task_t *task)
{
        return task->state;
}

worker_t *
task_get_cur_worker(task_t *task)
{
        return task->worker;
}

void
task_set_cur_worker(task_t *task, worker_t *worker)
{
        task->worker = worker;
}

int
task_init(task_t *task, int task_id, int priority, int wtime_sec,
          task_handler_fn_t fn, void *pvt_data)
{
        if (!task) {
                return -EINVAL;
        }

        task->task_id = task_id;
        INIT_LIST_HEAD(&task->list);
        queue_item_init(&task->qitem);
        task->fn = fn;
        task->pvt_data = pvt_data;
        task->priority = priority;
        task->wtime_sec = wtime_sec;
        task->state = TASK_STATE_IDLE;
        task->worker = NULL;

        return 0;
}

void
task_term(task_t *task)
{
        return;
}
