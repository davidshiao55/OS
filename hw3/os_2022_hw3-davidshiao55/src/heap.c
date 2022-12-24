#include <stdio.h>
#include <stdlib.h>
#include "../include/heap.h"

#define MAX_ELEMENT 200
#define HEAP_EMPTY(N) N == -1
#define HEAP_FULL(N) (N == MAX_ELEMENT - 1)

MinHeap *create_heap()
{
    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    heap->n = -1;
    heap->size = MAX_ELEMENT;
    heap->elementsArray = (heapElement *)malloc(sizeof(heapElement) * heap->size);
    return heap;
}

void destroy_heap(MinHeap *heap)
{
    free(heap->elementsArray);
    free(heap);
    heap = NULL;
}

void change(MinHeap *heap, int x, int y)
{
    heapElement *elementsArray = heap->elementsArray;
    int *n = &(heap->n);

    if (x > *n || x < 0)
    {
        printf("out of range\n");
        return;
    }
    int pos = x;
    heapElement tmp = elementsArray[x];
    tmp.priority = y;
    if (y > elementsArray[x].priority) // 往下沉
    {
        int parent, child;
        parent = x;
        child = 2 * x + 1;
        while (child <= *n)
        {
            if (child < *n && elementsArray[child].priority > elementsArray[child + 1].priority)
                child++;
            if (tmp.priority <= elementsArray[child].priority)
                break;
            elementsArray[parent] = elementsArray[child];
            parent = child;
            child = parent * 2 + 1;
        }
        pos = parent;
    }
    else
    {
        int i = x;
        while ((i != 0) && (tmp.priority < elementsArray[(i - 1) / 2].priority))
        {
            elementsArray[i] = elementsArray[(i - 1) / 2];
            i--;
            i /= 2;
        }
        pos = i;
    }
    elementsArray[pos] = tmp;
}

void heap_push(MinHeap *heap, Task *t)
{
    heapElement item;
    item.priority = t->priority;
    item.t = t;
    heapElement *elementsArray = heap->elementsArray;
    int *n = &(heap->n);

    if (HEAP_FULL(*n))
    {
        printf("HEAP full. \n");
        return;
    }

    int i;
    i = ++(*n);
    while ((i != 0) && (item.priority < elementsArray[(i - 1) / 2].priority))
    {
        elementsArray[i] = elementsArray[(i - 1) / 2];
        i--;
        i /= 2;
    }
    elementsArray[i] = item;
}

heapElement heap_pop(MinHeap *heap)
{
    heapElement *elementsArray = heap->elementsArray;
    int *n = &(heap->n);

    if (HEAP_EMPTY(*n))
    {
        printf("HEAP empty\n");
        heapElement empty;
        empty.priority = -1;
        return empty;
    }
    int parent, child;
    heapElement item, tmp;
    item = elementsArray[0];
    tmp = elementsArray[(*n)--];
    parent = 0;
    child = 1;
    while (child <= *n)
    {
        if (child < *n && elementsArray[child].priority > elementsArray[child + 1].priority)
            child++;
        if (tmp.priority <= elementsArray[child].priority)
            break;
        elementsArray[parent] = elementsArray[child];
        parent = child;
        child = parent * 2 + 1;
    }
    elementsArray[parent] = tmp;
    return item;
}
