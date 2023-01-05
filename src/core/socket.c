#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#define MAX_PACKET_SIZE 1024 

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#include "headers.h"

#if PLATFORM == PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#if PLATFORM == PLATFORM_WINDOWS
    #pragma comment( lib, "wsock32.lib" )
#endif

#include "socket.h"

bool socket_initalize()
{
#if PLATFORM == PLATFORM_WINDOWS
    WSADATA WsaData;
    return WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
#else
    return true;
#endif
}

void socket_shutdown()
{
#if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
#endif
}

bool socket_create(int* socket_handle)
{
    *socket_handle = socket(AF_INET, SOCK_DGRAM, 0);

    if(*socket_handle <= 0 )
    {
        printf("Failed to create socket.\n");
        return false;
    }

    return true;
}

void socket_close(int socket_handle)
{
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    close(socket_handle);
#elif PLATFORM == PLATFORM_WINDOWS
    closesocket(socket_handle);
#endif
}

bool socket_bind(int socket_handle, Address* address, uint16_t port)
{
    struct sockaddr_in to = {0};
 
    to.sin_family = AF_INET;

    if(address == NULL)
    {
        to.sin_addr.s_addr = htonl(INADDR_ANY); // server
    }
    else
    {
        uint32_t address_uint32_t = (address->a << 24) | (address->b << 16) | (address->c << 8) | (address->d);
        to.sin_addr.s_addr = htonl(address_uint32_t);
    }

    to.sin_port = htons(port);

    if (bind(socket_handle,(const struct sockaddr*) &to, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Failed to bind socket.\n");
        return false;
    }

    return true;
}

int socket_sendto(int socket_handle, Address* address, uint8_t* pkt, uint32_t pkt_size)
{
    struct sockaddr_in to = {0};

    uint32_t address_uint32_t = (address->a << 24) | (address->b << 16) | (address->c << 8) | (address->d);

    to.sin_family      = AF_INET;
    to.sin_addr.s_addr = htonl(address_uint32_t);
    to.sin_port        = htons(address->port);

    int sent_bytes = sendto(socket_handle,(const uint8_t*)pkt, pkt_size, 0, (struct sockaddr*)&to, sizeof(struct sockaddr_in));

    if (sent_bytes != pkt_size)
    {
        perror("Failed to send packet.\n");
        return 0;
    }
    
    return sent_bytes;
}

int socket_recvfrom(int socket_handle, Address* address, uint8_t* pkt)
{
    uint8_t packet_data[MAX_PACKET_SIZE] = {0};

    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);

    int recv_bytes = recvfrom(socket_handle, (uint8_t*)&packet_data, MAX_PACKET_SIZE, 0, (struct sockaddr*)&from, &from_len);

    memcpy(pkt,packet_data,recv_bytes);

    address->a = (uint8_t)(from.sin_addr.s_addr >> 0);
    address->b = (uint8_t)(from.sin_addr.s_addr >> 8);
    address->c = (uint8_t)(from.sin_addr.s_addr >> 16);
    address->d = (uint8_t)(from.sin_addr.s_addr >> 24);
    address->port = ntohs(from.sin_port);

    if (recv_bytes < 0 )
    {
        perror("Failed to receive packet.\n" );
        return 0;
    }

    return recv_bytes;
}
