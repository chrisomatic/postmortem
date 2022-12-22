#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint16_t port;
} __attribute__((__packed__)) Address;

bool socket_initalize();
void socket_shutdown();

bool socket_create(int* socket_handle);
bool socket_bind(int socket_handle, Address* address, uint16_t port);
void socket_close(int socket_handle);

int socket_sendto(int socket_handle, Address* address, uint8_t* pkt, uint32_t pkt_size);
int socket_recvfrom(int socket_handle, Address* address, uint8_t* pkt);
