#include <stdio.h>
#include <stdlib.h>
#include "../include/queue.h"

QNode *newNode(Task *t)
{
    struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
    temp->t = t;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty queue
Queue *createQueue()
{
    struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

void detroyQueue(Queue *q, bool delete)
{
    while (q->front)
    {
        if (delete)
            destroyTask(q->front->t);

        deQueue(q);
    }
    free(q);
}

// The function to add a key k to q
void enQueue(struct Queue *q, Task *t)
{
    // Create a new LL node
    struct QNode *temp = newNode(t);

    // If queue is empty, then new node is front and rear
    // both
    if (q->front == NULL)
    {
        q->front = q->rear = temp;
        return;
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}

void priorityenQueue(Queue *q, Task *t)
{
}

// Function to remove a key from given queue q
void deQueue(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;

    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    free(temp);
}

void deleteQueue(Queue *q, Task *t)
{
    QNode *pre = NULL;
    QNode *tmp = q->front;
    while (tmp)
    {
        if (tmp->t == t)
        {
            break;
        }
        pre = tmp;
        tmp = tmp->next;
    }
    if (tmp == NULL)
        printf("TASK DOESN'T EXIST\n");
    else if (tmp == q->front)
    {
        q->front = tmp->next;
    }
    else if (tmp == q->rear)
    {
        q->rear = pre;
        pre->next = tmp->next;
    }
    else
    {
        pre->next = tmp->next;
    }
    if (q->front == NULL)
        q->rear = NULL;
    free(tmp);
}
