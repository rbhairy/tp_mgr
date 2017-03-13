#ifndef QUEUE_H_
#define QUEUE_H_

#define NOT_IN_QUEUE ((queue_item_t*) -1)

typedef struct queue_item_ {
    struct queue_item_ *next;
} queue_item_t;

typedef struct queue_ {
    queue_item_t *head;
    queue_item_t *tail;
} queue_t;

static inline void
queue_init(queue_t *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

static inline void
queue_term(queue_t *queue)
{
        queue->head = NULL;
        queue->tail = NULL;
}

static inline int
queue_empty(queue_t *queue)
{
        return queue->head ? 0 : 1;
}

static inline void
queue_push(queue_t *queue, queue_item_t *queue_item)
{
        queue_item->next = NULL;
        if (!queue->head)
                queue->head = queue_item;
        if (queue->tail)
                queue->tail->next = queue_item;
        queue->tail = queue_item;
}

static inline void
queue_push_order(queue_t *queue, queue_item_t *queue_item,
                int (*compare)(queue_item_t *, queue_item_t *))
{
        queue_item_t *cur_qitem, *prev_qitem;

        queue_item->next = NULL;
        if (!queue->head && !queue->tail) {
                queue->head = queue_item;
                queue->tail = queue_item;
                return;
        }

        //insert before head
        if (compare(queue_item, queue->head) > 0) {
                queue_item->next = queue->head;
                queue->head = queue_item;
                return;
        }

        //insert after tail
        if (compare(queue_item, queue->tail) <= 0) {
                queue->tail = queue_item;
                queue_item->next = NULL;
                return;
        }

        //insert inbetween
        prev_qitem = queue->head;
        cur_qitem = queue->head->next;
        while (cur_qitem != NULL) {
                if (compare(queue_item, cur_qitem) > 0)
                        break;
                prev_qitem = cur_qitem;
                cur_qitem = cur_qitem->next;
        }
        queue_item->next = cur_qitem;
        prev_qitem->next = queue_item;
        return;
}

static inline queue_item_t *
queue_pop (queue_t *queue)
{
        queue_item_t *result;

        if (!queue->head)
                return NULL;
        result = queue->head;
        queue->head = result->next;
        if (!queue->head)
                queue->tail = NULL;
        result->next = NOT_IN_QUEUE;
        return result;
}

static inline void
queue_item_init (queue_item_t *queue_item)
{
        queue_item->next = NOT_IN_QUEUE;
}

#endif /* QUEUE_H_ */
