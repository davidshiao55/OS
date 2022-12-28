#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ucontext.h>
#include <stdbool.h>
#include <pthread.h>
#include "../include/task.h"
#include "../include/function.h"
#include "../include/queue.h"

Queue *ready_queue;
Queue *task_queue;
Queue *waiting_queue;

Task *curr_task = NULL;
ucontext_t ctx_main;
int taskCount = 0;
int algorithm;
bool sim = false;
void (*funcs[9])() = {task1, task2, task3, task4, task5, task6, task7, task8, task9};

void my_alarm_handler(int sig)
{
    if (!sim)
        return;
    static int rr_count = 0;
    if (curr_task)
    {
        curr_task->running_time++;
        if (algorithm == RR)
        {
            rr_count++;
            if (rr_count == 3)
            {
                rr_count = 0;
                curr_task->task_state = READY;
                enQueue(ready_queue, curr_task);
                swapcontext(&(curr_task->task_context->ctx), &ctx_main);
            }
        }
    }

    QNode *rNode = ready_queue->front;
    while (rNode)
    {
        rNode->t->waiting_time++;
        rNode = rNode->next;
    }

    QNode *wNode = waiting_queue->front;
    while (wNode)
    {
        Task *t = wNode->t;
        t->waiting_time++;
        if (t->sleep_time > 0)
        {
            t->sleep_time--;
            if (t->sleep_time <= 0)
            {
                printf("waking up task %s\n", wNode->t->task_name);
                t->task_state = READY;
                if (algorithm == PP)
                    priorityenQueue(ready_queue, t);
                else
                    enQueue(ready_queue, t);
                deleteQueue(waiting_queue, t);
                break;
            }
        }
        else if (t->waiting_resources)
        {
            if (check_resources(t->waiting_resources_count, t->waiting_resources))
            {
                printf("waking up task %s\n", wNode->t->task_name);
                t->task_state = READY;
                if (algorithm == PP)
                    priorityenQueue(ready_queue, t);
                else
                    enQueue(ready_queue, t);
                deleteQueue(waiting_queue, t);
                break;
            }
        }
        wNode = wNode->next;
    }
}

void my_stp_handler(int sig)
{
    // stop simulation
    if (sim)
    {
        sim = false;
        if (curr_task)
        {
            swapcontext(&(curr_task->task_context->ctx), &ctx_main);
        }
    }
}

void task_manager_initialize(const char *inputString)
{
    if (!strcmp(inputString, "FCFS"))
        algorithm = FCFS;
    else if (!strcmp(inputString, "RR"))
        algorithm = RR;
    else if (!strcmp(inputString, "PP"))
        algorithm = PP;

    task_queue = createQueue();
    waiting_queue = createQueue();
    ready_queue = createQueue();

    signal(SIGTSTP, my_stp_handler);
}

void task_manager_destroy()
{
    detroyQueue(ready_queue, false);
    detroyQueue(waiting_queue, false);
    detroyQueue(task_queue, true);
}

void startSimulation()
{
    pthread_t tid;
    sim = true;
    printf("Start simulation.\n");
    pthread_create(&tid, NULL, simulation, NULL);
    pthread_join(tid, NULL);
    printf("HI\n");
}

void *simulation(void *vargp)
{
    /* set timer */
    struct itimerval tv;
    tv.it_interval.tv_usec = 10000;
    tv.it_interval.tv_sec = 0;
    tv.it_value.tv_usec = 10000;
    tv.it_value.tv_sec = 0;

    if (setitimer(ITIMER_REAL, &tv, NULL) < 0)
    {
        perror("settimer error.\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, my_alarm_handler);

    while (sim)
    {
        if (ready_queue->front)
        {
            curr_task = ready_queue->front->t;
            curr_task->task_state = RUNNING;
            deQueue(ready_queue);
            printf("Task %s is running.\n", curr_task->task_name);
            swapcontext(&ctx_main, &(curr_task->task_context->ctx));
            /* if task is terminated than release context & stack : seg fault*/
            // if (curr_task->task_state == TERMINATED)
            //     deleteContext(curr_task);
            curr_task = NULL;
        }
        else
        {
            if (!waiting_queue->front)
            {
                break;
            }
            /* ready q empty but waiting queue isn't */
            else
            {
                printf("CPU idle\n");
                while (waiting_queue->front && !ready_queue->front && sim)
                {
                }
            }
        }
    }
    printf("Simulation over.\n");

    // clear timer
    tv.it_value.tv_usec = 0;
    tv.it_value.tv_sec = 0;
    if (setitimer(ITIMER_REAL, &tv, NULL) < 0)
    {
        perror("settimer error.\n");
        exit(EXIT_FAILURE);
    }
    return NULL;
}

void task_wait_resource(int count, int *resources)
{
    printf("Task %s is waiting resource.\n", curr_task->task_name);
    curr_task->waiting_resources_count = count;
    curr_task->waiting_resources = (int *)malloc(sizeof(int) * count);
    for (int j = 0; j < count; j++)
        curr_task->waiting_resources[j] = resources[j];
    curr_task->task_state = WAITING;
    enQueue(waiting_queue, curr_task);
    swapcontext(&(curr_task->task_context->ctx), &ctx_main);
}

void task_sleep(int ms)
{
    printf("Task %s goes to sleep.\n", curr_task->task_name);
    curr_task->task_state = WAITING;
    curr_task->sleep_time = ms / 10;
    enQueue(waiting_queue, curr_task);
    swapcontext(&(curr_task->task_context->ctx), &ctx_main);
}

void task_exit()
{
    printf("Task %s is killed.\n", curr_task->task_name);
    curr_task->task_state = TERMINATED;
    curr_task->turnaround = curr_task->running_time + curr_task->waiting_time;
    swapcontext(&(curr_task->task_context->ctx), &ctx_main);
}

void addTask(char *taskName, char *functionName, int priority)
{
    Task *t = newTask(taskName, functionName, priority);
    switch (algorithm)
    {
    case FCFS:
        enQueue(task_queue, t);
        enQueue(ready_queue, t);
        break;
    case RR:
        enQueue(task_queue, t);
        enQueue(ready_queue, t);
        break;
    case PP:
        enQueue(task_queue, t);
        priorityenQueue(ready_queue, t);
        break;
    }
    printf("Task %s is ready.\n", t->task_name);
}

void deleteTask(char *taskName)
{
    QNode *tmp = task_queue->front;
    while (tmp)
    {
        if (!strcmp(tmp->t->task_name, taskName))
            break;
        tmp = tmp->next;
    }
    if (!tmp)
        return;
    if (tmp->t->task_state == RUNNING)
    {
        task_exit();
    }
    else if (tmp->t->task_state == WAITING)
    {
        tmp->t->task_state = TERMINATED;
        deleteQueue(waiting_queue, tmp->t);
    }
    else if (tmp->t->task_state == READY)
    {
        tmp->t->task_state = TERMINATED;
        deleteQueue(ready_queue, tmp->t);
    }
}

void printTask()
{
    printf(" TID|\tname|\t     state|\trunning|\twaiting|\tturnaround|\t  resources|\tpriority\n");
    for (int i = 0; i < 107; i++)
        printf("-");
    printf("\n");
    QNode *tmp = task_queue->front;
    while (tmp)
    {
        taskInfo(tmp->t);
        tmp = tmp->next;
    }
}

void taskInfo(Task *t)
{
    printf("%4d|", t->tid);
    printf("%7s|", t->task_name);
    switch (t->task_state)
    {
    case READY:
        printf("        READY|");
        break;
    case TERMINATED:
        printf("   TERMINATED|");
        break;
    case WAITING:
        printf("      WAITING|");
        break;
    case RUNNING:
        printf("      RUNNING|");
        break;
    }
    printf("\t%7d|", t->running_time);
    printf("\t%7d|", t->waiting_time);
    if (t->turnaround)
        printf("\t%10d|", t->turnaround);
    else
        printf("\t      none|");
    char buffer[18] = "                |";
    int pos = 15;
    for (int i = 0; i < MAX_RESOURCE; i++)
    {
        if (t->resources[i])
        {
            buffer[pos--] = i + '0';
            buffer[pos--] = ' ';
        }
    }
    if (pos == 15)
        strcpy(buffer, "            none|");
    printf("%s", buffer);
    printf("\t%8d\n", t->priority);
}

Task_context *newContext(int f)
{
    Task_context *tc = (Task_context *)malloc(sizeof(Task_context));
    getcontext(&tc->ctx);
    tc->ctx.uc_stack.ss_sp = tc->stack;
    tc->ctx.uc_stack.ss_size = sizeof(tc->stack);
    tc->ctx.uc_stack.ss_flags = 0;
    tc->ctx.uc_link = &ctx_main;
    makecontext(&tc->ctx, funcs[f - 1], 0);
    return tc;
}

void deleteContext(Task *t)
{
    printf("DELETE task context %s\n", t->task_name);
    free(t->task_context);
    t->task_context = NULL;
}

Task *newTask(char *taskName, char *functionName, int priority)
{
    Task *t = (Task *)malloc(sizeof(Task));
    t->tid = ++taskCount;
    t->task_name = malloc(sizeof(char) * strlen(taskName) + 1);
    strcpy(t->task_name, taskName);
    t->function = functionName[4] - '0';
    t->priority = priority;
    t->running_time = 0;
    t->waiting_time = 0;
    t->turnaround = 0;
    t->task_state = READY;
    t->task_context = newContext(t->function);
    for (int i = 0; i < MAX_RESOURCE; i++)
        t->resources[i] = false;
    t->sleep_time = 0;
    t->waiting_resources = NULL;
    t->waiting_resources_count = 0;
    return t;
}

void destroyTask(Task *t)
{
    if (t->task_context)
        free(t->task_context);
    if (t->waiting_resources)
        free(t->waiting_resources);
    free(t->task_name);
    free(t);
}
