#pragma once

#define MAX_CLIENTS 8
#define MAX_PACKET_DATA_SIZE 1024

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

typedef struct
{
    uint32_t game_id;
    uint16_t id;
    uint16_t ack;
    uint32_t ack_bitfield;
    uint8_t type;
    uint8_t pad[3]; // pad to be 4-byte aligned
} __attribute__((__packed__)) PacketHeader;

typedef struct
{
    PacketHeader hdr;

    uint32_t data_len;
    uint8_t  data[MAX_PACKET_DATA_SIZE];
} __attribute__((__packed__)) Packet;

extern char* server_ip_address;

// Server
int net_server_start();

// Client
bool net_client_init();
bool net_client_connect();
void net_client_update();
bool net_client_add_player_input(uint16_t input, double game_time);
bool net_client_is_connected();
void net_client_disconnect();
bool net_client_set_server_ip(char* address);
bool net_client_data_waiting();
int net_client_send(uint8_t* data, uint32_t len);
int net_client_recv(Packet* pkt, bool* is_latest);
void net_client_deinit();
