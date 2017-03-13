#ifndef WORKER_H_
#define WORKER_H_

#include <pthread.h>
#include <ucontext.h>

#define WORKER_NAME_LEN 32

typedef enum worker_event_type_ {
        WORKER_ETYPE_PAUSE = 0,
        WORKER_ETYPE_STOP,
        WORKER_ETYPE_RUN,
        WORKER_ETYPE_INVALID
} worker_event_type_t;

typedef struct worker_ {
        char worker_name[WORKER_NAME_LEN];
        pthread_t thread;

        //stop, pause
        worker_event_type_t event;
        pthread_mutex_t mutex;
        pthread_cond_t cond;

        int action;
        //worker main context
        ucontext_t ctx;
        void *wpool;
} worker_t;

int worker_init(worker_t *worker, char *worker_name, void *wpool);
void worker_pause(worker_t *worker);
void worker_stop(worker_t *worker);
void worker_should_yield(void);

#endif /* WORKER_H_ */
