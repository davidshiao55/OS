#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>

#define MAX_RESOURCE 8

void get_resources(int, int *);
void release_resources(int, int *);
bool check_resources(int count, int *resources);

#endif
