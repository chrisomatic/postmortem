typedef struct
{
    int max_count;
    int count;
    size_t item_size;
    void* buf;
} CircBuf;

void circbuf_create(CircBuf* cb, int max_count, size_t item_size)
{
    cb->item_size = item_size;
    cb->count = 0;
    cb->max_count = max_count;
    cb->buf = malloc(item_size*max_count);
}

void circbuf_delete(CircBuf* cb)
{
    if(cb->buf)
    {
        free(cb->buf);
        cb->buf = NULL;
    }
    cb->count = 0;
}

void circbuf_add(CircBuf* cb, void* item)
{
    int index = cb->count;

    if(cb->count == cb->max_count)
    {
        // shift
        for(int i = 1; i <= cb->max_count-1; ++i)
            memcpy(cb->buf+(i-1),cb->buf+i,cb->item_size);

        index--;
    }
    else
    {
        cb->count++;
    }

    void* it = (cb->buf+index);
    memcpy(it,item,cb->item_size);
}

void* circbuf_get_item(CircBuf* cb,int index)
{
    if(index < 0 || index >= cb->max_count)
        return NULL;

    return (cb->buf+index);
}
