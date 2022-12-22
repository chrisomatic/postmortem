#pragma once

typedef struct
{
    Packet* data;
    uint32_t front,rear,size;
    uint32_t capacity;
} PacketQueue;

bool packet_queue_create(PacketQueue* q, uint32_t capacity)
{
    q->data = malloc(capacity*sizeof(Packet));
    if(!q->data)
        return false;

    q->capacity = capacity;
    q->front = 0;
    q->size = 0;
    q->rear = capacity-1;

    return true;
}

bool packet_queue_is_full(PacketQueue* q)
{
    return (q->size == q->capacity);
}

bool packet_queue_is_empty(PacketQueue* q)
{
    return (q->size == 0);
}

bool packet_queue_enqueue(PacketQueue* q, Packet* p)
{
    if(packet_queue_is_full(q))
        return false;

    q->rear++;
    q->rear %= q->capacity;
    memcpy(&q->data[q->rear],p,sizeof(Packet));
    q->size++;

    return true;
}

bool packet_queue_dequeue(PacketQueue* q, Packet* p)
{
    if(packet_queue_is_empty(q))
        return false;

    memcpy(p,&q->data[q->front],sizeof(Packet));
    q->front++;
    q->front %= q->capacity;
    q->size--;

    return true;
}
