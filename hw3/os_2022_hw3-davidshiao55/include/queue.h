#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include "task.h"

typedef struct QNode
{
    Task *t;
    struct QNode *next;
} QNode;

typedef struct Queue
{
    struct QNode *front, *rear;
} Queue;

QNode *newNode(Task *t);
Queue *createQueue();
void detroyQueue(Queue *q, bool delete);
void enQueue(Queue *q, Task *t);
void priorityenQueue(Queue *q, Task *t);
void deQueue(Queue *q);
void deleteQueue(Queue *q, Task *t);

#endif