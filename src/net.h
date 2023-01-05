#pragma once

#include "math2d.h"

#define TICK_RATE 20.0f

#define MAX_CLIENTS 8
#define MAX_PACKET_DATA_SIZE 1024

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif


typedef enum
{
    PACKET_TYPE_INIT = 0,
    PACKET_TYPE_CONNECT_REQUEST,
    PACKET_TYPE_CONNECT_CHALLENGE,
    PACKET_TYPE_CONNECT_CHALLENGE_RESP,
    PACKET_TYPE_CONNECT_ACCEPTED,
    PACKET_TYPE_CONNECT_REJECTED,
    PACKET_TYPE_DISCONNECT,
    PACKET_TYPE_PING,
    PACKET_TYPE_INPUT,
    PACKET_TYPE_STATE,
    PACKET_TYPE_ERROR,
} PacketType;

typedef enum
{
    DISCONNECTED = 0,
    SENDING_CONNECTION_REQUEST,
    SENDING_CHALLENGE_RESPONSE,
    CONNECTED,
} ConnectionState;

typedef enum
{
    CONNECT_REJECT_REASON_SERVER_FULL,
    CONNECT_REJECT_REASON_INVALID_PACKET,
    CONNECT_REJECT_REASON_FAILED_CHALLENGE,
} ConnectionRejectionReason;

typedef enum
{
    PACKET_ERROR_NONE = 0,
    PACKET_ERROR_BAD_FORMAT,
    PACKET_ERROR_INVALID,
} PacketError;

PACK(struct PacketHeader
{
    uint32_t game_id;
    uint16_t id;
    uint16_t ack;
    uint32_t ack_bitfield;
    uint8_t type;
    uint8_t pad[3]; // pad to be 4-byte aligned
});

typedef struct PacketHeader PacketHeader;

PACK(struct Packet
{
    PacketHeader hdr;

    uint32_t data_len;
    uint8_t  data[MAX_PACKET_DATA_SIZE];
});

typedef struct Packet Packet;

PACK(struct NetPlayerInput
{
    double delta_t;
    uint32_t keys;
    int mouse_x;
    int mouse_y;
});

typedef struct NetPlayerInput NetPlayerInput;

PACK(struct PlayerNetState
{
    bool active;
    uint16_t associated_packet_id;
    Vector2f pos;
    float angle;
    uint8_t sprite_index;
});

typedef struct PlayerNetState PlayerNetState;

extern char* server_ip_address;

// Server
int net_server_start();

// Client
bool net_client_init();
int net_client_connect();
void net_client_update();
uint8_t net_client_get_player_count();
int net_client_get_input_count();
uint16_t net_client_get_latest_local_packet_id();
bool net_client_add_player_input(NetPlayerInput* input);
bool net_client_is_connected();
void net_client_disconnect();
bool net_client_set_server_ip(char* address);
void net_client_get_server_ip_str(char* ip_str);
bool net_client_data_waiting();
double net_client_get_rtt();
int net_client_send(uint8_t* data, uint32_t len);
int net_client_recv(Packet* pkt);
void net_client_deinit();
