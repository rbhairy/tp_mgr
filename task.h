#ifndef TASK_H_
#define TASK_H_

#include <ucontext.h>

#include "list.h"
#include "queue.h"
#include "task_timer.h"

#include "worker.h"

typedef void (*task_handler_fn_t) (void *private_data);

typedef enum task_state_ {
        TASK_STATE_IDLE = 0,
        TASK_STATE_RUNNING,
        TASK_STATE_PAUSED,
        TASK_STATE_STOPPED,
        TASK_STATE_COMPLETED,
        TASK_STATE_INVALID
} task_state_t;


typedef struct task_ {
        int task_id;
        task_state_t state;
        int priority;
        int wtime_sec;
        task_timer_t timer;

        //global list anchor
        struct list_head list;

        //queue anchor
        queue_item_t qitem;

        //user supplied fn and pvt_data
        task_handler_fn_t fn;
        void *pvt_data;

        //save context for pause/resume
        ucontext_t ctx;

        //worker which is running this task
        worker_t *worker;
} task_t;

char *get_task_state_str(task_state_t task_state);
int task_init(task_t *task, int task_id, int priority, int wtime_sec,
              task_handler_fn_t fn, void *pvt_data);
void task_set_state(task_t *task, task_state_t new_state);
task_state_t task_get_state(task_t *task);
worker_t *task_get_cur_worker(task_t *task);
void task_set_cur_worker(task_t *task, worker_t *worker);
#endif  /* TASK_H_ */
