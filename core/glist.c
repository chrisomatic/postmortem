#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "log.h"
#include "glist.h"

glist* list_create(void* buf, int max_count, int item_size)
{
    if(item_size <= 0 || max_count <= 1)
    {
        LOGE("Invalid item_size (%d) or max_count (%d) for list", item_size, max_count);
        return NULL;
    }
    if(buf == NULL)
    {
        LOGE("List buffer is NULL");
        return NULL;
    }

    glist* list = calloc(1, sizeof(glist));
    list->count = 0;
    list->max_count = max_count;
    list->item_size = item_size;
    list->buf = buf;
    // if(list->buf == NULL)
    // {
    //     LOGI("Allocating %d bytes for list %p", max_count*item_size, list);
    //     list->buf = calloc(max_count, item_size);
    // }
    return list;
}

void list_delete(glist* list)
{
    if(list != NULL) free(list);
    list = NULL;
}

bool list_add(glist* list, void* item)
{
    if(list == NULL)
        return false;

    if(list_is_full(list))
        return false;

    memcpy(list->buf + list->count*list->item_size, item, list->item_size);
    list->count++;
    return true;
}

bool list_remove(glist* list, int index)
{
    if(list == NULL)
        return false;

    if(index >= list->count)
        return false;

    memcpy(list->buf + index*list->item_size, list->buf+(list->count-1)*list->item_size, list->item_size);
    list->count--;
}

bool list_remove_by_item(glist* list, void* item)
{
    if(list == NULL)
        return false;

    if(item == NULL)
        return false;

    memcpy(item, list->buf+(list->count-1)*list->item_size, list->item_size);
    list->count--;
}

bool list_is_full(glist* list)
{
    return (list->count >= list->max_count);
}

bool list_is_empty(glist* list)
{
    return (list->count == 0);
}

void* list_get(glist* list, int index)
{
    if(list == NULL)
        return NULL;

    return list->buf + index*list->item_size;
}
