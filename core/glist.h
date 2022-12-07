#pragma once 

typedef struct
{
    int max_count;
    int count;
    int item_size;
    void* buf;
} glist;

glist* list_create(void* buf, int max_count, int item_size);
void list_delete(glist* list);
bool list_add(glist* list, void* item);
bool list_remove(glist* list, int index);
bool list_remove_by_item(glist* list, void* item);
void* list_get(glist* list, int index);
bool list_is_full(glist* list);
bool list_is_empty(glist* list);
