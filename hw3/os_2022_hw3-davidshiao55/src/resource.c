#include <stdio.h>
#include <stdlib.h>
#include "../include/resource.h"
#include "../include/task.h"

bool resourse_available[MAX_RESOURCE] = {true, true, true, true, true, true, true, true};

void get_resources(int count, int *resources)
{
    int i = 0;
    while (!check_resources(count, resources))
    {
        task_wait_resource(count, resources);
    }
    for (i = 0; i < count; i++)
    {
        printf("Task %s gets resource %d.\n", curr_task->task_name, resources[i]);
        curr_task->resources[resources[i]] = true;
        resourse_available[resources[i]] = false;
    }
    if (curr_task->waiting_resources)
    {
        free(curr_task->waiting_resources);
        curr_task->waiting_resources = NULL;
        curr_task->waiting_resources_count = 0;
    }
}

void release_resources(int count, int *resources)
{
    for (int i = 0; i < count; i++)
    {
        printf("Task %s releases resource %d.\n", curr_task->task_name, resources[i]);
        curr_task->resources[resources[i]] = false;
        resourse_available[resources[i]] = true;
    }
}

bool check_resources(int count, int *resources)
{
    for (int i = 0; i < count; i++)
    {
        if (!resourse_available[resources[i]])
            return false;
    }
    return true;
}
