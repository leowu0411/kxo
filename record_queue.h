#ifndef LAB0_QUEUE_H
#define LAB0_QUEUE_H

/* This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a circular doubly-linked list to represent the set of queue elements
 */

#include <stdbool.h>
#include <stddef.h>

#include "list.h"

/**
 * element_t - Linked list element
 * @value: pointer to array holding string
 * @list: node of a doubly-linked list
 *
 * @value needs to be explicitly allocated and freed
 */
typedef struct {
    int *value;
    int len;
    char ai;
    struct list_head list;
} element_t;

/* Operations on queue */

/**
 * q_new() - Create an empty queue whose next and prev pointer point to itself
 *
 * Return: NULL for allocation failed
 */
struct list_head *q_new();

/**
 * q_free() - Free all storage used by queue, no effect if header is NULL
 * @head: header of queue
 */
void q_free(struct list_head *head);

bool q_insert_head(struct list_head *head, const int *move, int len, char ai);

#endif /* LAB0_QUEUE_H */
