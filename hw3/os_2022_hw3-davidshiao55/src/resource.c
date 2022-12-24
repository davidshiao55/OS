#include <stdio.h>
#include "../include/resource.h"
#include "../include/task.h"

bool resourse_available[MAX_RESOURCE] = {true, true, true, true, true, true, true, true};

void get_resources(int count, int *resources)
{
    // int i = 0;
    // while (1)
    // {
    //     for (i = 0; i < count; i++)
    //     {
    //         if (!resourse_available[resources[i]])
    //             break;
    //     }
    //     if (i == count)
    //         break;
    //     else
    //     {
    //         printf("Task %s is waiting resource.\n", curr_task->task_name);
    //         task_sleep(30);
    //     }
    // }
    // for (i = 0; i < count; i++)
    // {
    //     printf("Task %s gets resource %d.\n", curr_task->task_name, resources[i]);
    //     curr_task->resources[resources[i]] = true;
    //     resourse_available[resources[i]] = false;
    // }
}

void release_resources(int count, int *resources)
{
    // for (int i = 0; i < count; i++)
    // {
    //     printf("Task %s releases resource %d.\n", curr_task->task_name, resources[i]);
    //     curr_task->resources[resources[i]] = false;
    //     resourse_available[resources[i]] = true;
    // }
}
