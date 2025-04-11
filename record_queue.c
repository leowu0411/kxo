#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "record_queue.h"

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *ret = malloc(sizeof(struct list_head));
    if (ret)
        INIT_LIST_HEAD(ret);
    return ret;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    element_t *entry = NULL, *safe = NULL;
    // cppcheck-suppress unusedLabel
    list_for_each_entry_safe(entry, safe, head, list) {
        list_del(&entry->list);
        if (entry->value)
            free(entry->value);
        free(entry);
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, const int *move, int len, char ai)
{
    if (!head) {
        printf("Queue head is NULL\n");
        return false;
    }

    element_t *node = malloc(sizeof(element_t));
    if (!node) {
        printf("Failed to allocate memory for move\n");
        return false;
    }

    node->value = malloc(len * sizeof(int));
    if (!node->value) {
        free(node);
        printf("Failed to allocate memory for move\n");
        return false;
    }

    node->len = len;
    node->ai = ai;
    memcpy(node->value, move, len * sizeof(int));
    list_add(&node->list, head);
    return true;
}


