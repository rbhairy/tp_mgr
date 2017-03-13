#ifndef WORKER_POOL_H_
#define WORKER_POOL_H_

#include "task_timer.h"

//#include "worker.h"
#include "task.h"

#define WORKERS_MAX 16

void (*user_task_handler_fn) (void *private_data);

typedef struct worker_pool_ {
        int num_workers;
        worker_t workers[WORKERS_MAX];

        //next available task id for new task
        int next_task_id;

        //global mutext
        pthread_mutex_t mutex;
        pthread_cond_t cond;

        //global task list
        struct list_head task_list;

        //tasks which are running
        queue_t runq;

        //tasks which are pasued, waiting to be resumed
        queue_t pauseq;

        //tasks which will be scheduled after timeout
        task_timerset_t task_timerset;

        //poller thread to poll for new task to run or not
        pthread_t poller;
} worker_pool_t;

//worker pool realted routines
int worker_pool_init(worker_pool_t *wpool, int num_workers);
void worker_pool_term(worker_pool_t *wpool);

//work related routines
int add_utask(worker_pool_t *wpool, int priority, int wtime_sec,
             task_handler_fn_t fn, void *pvt_data);
int pause_utask(worker_pool_t *wpool, int task_id);
int stop_utask(worker_pool_t *wpool, int task_id);
int resume_utask(worker_pool_t *wpool, int task_id);
void print_utask_list(worker_pool_t *wpool);

//utask should yield the ctx or not
void yield_utask(void);

//routines for workers
task_t *pick_task_from_runq(worker_pool_t *wpool);
#endif /* WORKER_POOL_H_ */
