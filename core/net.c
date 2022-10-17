#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h> 
#include <time.h>
#include <sys/select.h>

#include "socket.h"
#include "timer.h"
#include "net.h"
#include "packet_queue.h"
#include "log.h"

#define GAME_ID 0xC68BB821

#define TICK_RATE 20.0f
#define PORT 27001

#define MAXIMUM_RTT 1.0f

#define DEFAULT_TIMEOUT 1.0f // seconds
#define PING_PERIOD 5.0f
#define DISCONNECTION_TIMEOUT 12.0f // seconds

typedef struct
{
    int socket;
    uint16_t local_latest_packet_id;
    uint16_t remote_latest_packet_id;
    PacketQueue latest_received_packets;
} NodeInfo;

// Info server stores about a client
typedef struct
{
    Address address;
    ConnectionState state;
    uint16_t remote_latest_packet_id;
    double  time_of_latest_packet;
    uint8_t client_salt[8];
    uint8_t server_salt[8];
    uint8_t xor_salts[8];
    ConnectionRejectionReason last_reject_reason;
    PacketError last_packet_error;
} ClientInfo;

struct
{
    Address address;
    NodeInfo info;
    ClientInfo clients[MAX_CLIENTS];
    int num_clients;
} server = {0};

// ---

#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

_Static_assert((RAND_MAX & (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

static uint64_t rand64(void)
{
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned) rand();
    }
    return r;
}

static Timer server_timer = {0};

static inline int get_packet_size(Packet* pkt)
{
    return (sizeof(pkt->hdr) + pkt->data_len + sizeof(pkt->data_len));
}

static inline bool is_packet_id_greater(uint16_t id, uint16_t cmp)
{
    return ((id > cmp) && (id - cmp <= 32768)) || 
           ((id < cmp) && (cmp - id  > 32768));
}

static char* packet_type_to_str(PacketType type)
{
    switch(type)
    {
        case PACKET_TYPE_INIT: return "INIT";
        case PACKET_TYPE_CONNECT_REQUEST: return "CONNECT REQUEST";
        case PACKET_TYPE_CONNECT_CHALLENGE: return "CONNECT CHALLENGE";
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP: return "CONNECT CHALLENGE RESP";
        case PACKET_TYPE_CONNECT_ACCEPTED: return "CONNECT ACCEPTED";
        case PACKET_TYPE_CONNECT_REJECTED: return "CONNECT REJECTED";
        case PACKET_TYPE_DISCONNECT: return "DISCONNECT";
        case PACKET_TYPE_PING: return "PING";
        case PACKET_TYPE_INPUT: return "INPUT";
        case PACKET_TYPE_STATE: return "STATE";
        case PACKET_TYPE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static char* connect_reject_reason_to_str(ConnectionRejectionReason reason)
{
    switch(reason)
    {
        case CONNECT_REJECT_REASON_SERVER_FULL: return "SERVER FULL";
        case CONNECT_REJECT_REASON_INVALID_PACKET: return "INVALID PACKET FORMAT";
        case CONNECT_REJECT_REASON_FAILED_CHALLENGE: return "FAILED CHALLENGE";
        default: return "UNKNOWN";
    }
}

static void print_salt(uint8_t* salt)
{
    LOGN("[SALT] %02X %02X %02X %02X %02X %02X %02X %02X",
            salt[0], salt[1], salt[2], salt[3],
            salt[4], salt[5], salt[6], salt[7]
    );
}

static void store_xor_salts(uint8_t* cli_salt, uint8_t* svr_salt, uint8_t* xor_salts)
{
    for(int i = 0; i < 8; ++i)
    {
        xor_salts[i] = (cli_salt[i] ^ svr_salt[i]);
    }
}

static void print_address(Address* addr)
{
    LOGN("[ADDR] %u.%u.%u.%u:%u",addr->a,addr->b,addr->c,addr->d,addr->port);
}

static void print_packet(Packet* pkt)
{
    LOGN("Game ID:      0x%08x",pkt->hdr.game_id);
    LOGN("Packet ID:    %u",pkt->hdr.id);
    LOGN("Packet Type:  %s (%02X)",packet_type_to_str(pkt->hdr.type),pkt->hdr.type);
    LOGN("Data (%u):", pkt->data_len);

    char data[3*16+5] = {0};
    char byte[4] = {0};
    for(int i = 0; i < MIN(16,pkt->data_len); ++i)
    {
        sprintf(byte,"%02X ",pkt->data[i]);
        memcpy(data+(3*i), byte,3);
    }

    if(pkt->data_len <= 16)
    {
        LOGN("%s", data);
    }
    else
    {
        LOGN("%s ...", data);
    }
}

static bool has_data_waiting(int socket)
{
    fd_set readfds;

    //clear the socket set  
    FD_ZERO(&readfds);
    
    //add client socket to set  
    FD_SET(socket, &readfds);

    int activity;

    struct timeval tv = {0};
    activity = select(socket + 1 , &readfds , NULL , NULL , &tv);

    if ((activity < 0) && (errno!=EINTR))
    {
        perror("select error");
        return false;
    }

    bool has_data = FD_ISSET(socket , &readfds);
    return has_data;
}

static int net_send(NodeInfo* node_info, Address* to, Packet* pkt)
{
    int pkt_len = get_packet_size(pkt);
    int sent_bytes = socket_sendto(node_info->socket, to, (uint8_t*)pkt, pkt_len);

    LOGN("[SENT] Packet %d (%u B)",pkt->hdr.id,sent_bytes);
    print_packet(pkt);

    node_info->local_latest_packet_id++;

    return sent_bytes;
}

static int net_recv(NodeInfo* node_info, Address* from, Packet* pkt, bool* is_latest)
{
    int recv_bytes = socket_recvfrom(node_info->socket, from, (uint8_t*)pkt);

    LOGN("[RECV] Packet %d (%u B)",pkt->hdr.id,recv_bytes);
    print_address(from);
    print_packet(pkt);

    *is_latest = is_packet_id_greater(pkt->hdr.id,node_info->remote_latest_packet_id);
    if(*is_latest)
    {
        node_info->remote_latest_packet_id = pkt->hdr.id;
    }

    //packet_queue_enqueue(&node_info->latest_received_packets,pkt);

    return recv_bytes;
}

static bool validate_packet_format(Packet* pkt)
{
    if(pkt->hdr.game_id != GAME_ID)
    {
        LOGN("Game ID of packet doesn't match, %08X != %08X",pkt->hdr.game_id, GAME_ID);
        return false;
    }

    if(pkt->hdr.type < PACKET_TYPE_INIT || pkt->hdr.type > PACKET_TYPE_STATE)
    {
        LOGN("Invalid Packet Type: %d", pkt->hdr.type);
        return false;
    }

    return true;
}

static bool authenticate_client(Packet* pkt, ClientInfo* cli)
{
    bool valid = true;

    switch(pkt->hdr.type)
    {
        case PACKET_TYPE_CONNECT_REQUEST:
            valid &= (pkt->data_len == MAX_PACKET_DATA_SIZE); // must be padded out to 1024
            valid &= (memcmp(&pkt->data[0],cli->client_salt, 8) == 0);
            break;
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
            valid &= (pkt->data_len == MAX_PACKET_DATA_SIZE); // must be padded out to 1024
            valid &= (memcmp(&pkt->data[0],cli->xor_salts, 8) == 0);
            break;
        default:
            valid &= (memcmp(&pkt->data[0],cli->xor_salts, 8) == 0);
            break;
    }

    return valid;
}

static bool server_get_client(Address* addr, ClientInfo** cli)
{
    for(int i = 0; i < server.num_clients; ++i)
    {
        if(memcmp(&server.clients[i].address, addr, sizeof(Address)) == 0)
        {
            // found existing client, exit
            *cli = &server.clients[i];
            return false;
        }
    }

    return true;
}

static bool server_assign_new_client(Address* addr, ClientInfo** cli)
{
    // new client
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state == DISCONNECTED)
        {
            *cli = &server.clients[i];
            return true;
        }
    }

    LOGN("Server is full and can't accept new clients.");
    return false;
}

static void update_server_num_clients()
{
    int num_clients = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state != DISCONNECTED)
        {
            num_clients++;
        }
    }
    server.num_clients = num_clients;
}

static void remove_client(ClientInfo* cli)
{
    LOGN("Remove client");
    memset(cli,0, sizeof(ClientInfo));
    update_server_num_clients();

}

static void server_send(PacketType type, ClientInfo* cli)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = server.info.local_latest_packet_id,
        .hdr.ack = server.info.remote_latest_packet_id,
        .hdr.type = type
    };

    switch(type)
    {
        case PACKET_TYPE_INIT:
            break;
        case PACKET_TYPE_CONNECT_CHALLENGE:
        {
            uint64_t salt = rand64();
            memcpy(cli->server_salt, (uint8_t*)&salt,8);

            // store xor salts
            store_xor_salts(cli->client_salt, cli->server_salt, cli->xor_salts);
            print_salt(cli->xor_salts);

            memcpy(&pkt.data[0],cli->client_salt,8);
            memcpy(&pkt.data[8],cli->server_salt,8);
            pkt.data_len = 16;

            net_send(&server.info,&cli->address,&pkt);

        }   break;
        case PACKET_TYPE_CONNECT_ACCEPTED:
        {
            cli->state = CONNECTED;
            pkt.data_len = 0;
            net_send(&server.info,&cli->address,&pkt);
        }   break;
        case PACKET_TYPE_CONNECT_REJECTED:
        {
            pkt.data_len = 1;
            pkt.data[0] = (uint8_t)cli->last_reject_reason;

            net_send(&server.info,&cli->address,&pkt);
        }   break;
        case PACKET_TYPE_PING:
            pkt.data_len = 0;
            net_send(&server.info,&cli->address,&pkt);
            break;
        case PACKET_TYPE_STATE:
            pkt.data_len = 0;
            net_send(&server.info,&cli->address,&pkt);
            break;
        case PACKET_TYPE_ERROR:
            pkt.data_len = 1;
            pkt.data[0] = (uint8_t)cli->last_packet_error;
            net_send(&server.info,&cli->address,&pkt);
            break;
        case PACKET_TYPE_DISCONNECT:
        {
            cli->state = DISCONNECTED;
            pkt.data_len = 0;
            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
                net_send(&server.info,&cli->address,&pkt);
        } break;
        default:
            break;
    }
}

int net_server_start()
{
    // init
    memset(server.clients, 0, sizeof(ClientInfo)*MAX_CLIENTS);
    server.num_clients = 0;

    int sock;

    // set tick rate
    timer_set_fps(&server_timer,TICK_RATE);
    timer_begin(&server_timer);

    LOGN("Creating socket.");
    socket_create(&sock);

    LOGN("Binding socket %u to any local ip on port %u.", sock, PORT);
    socket_bind(sock, NULL, PORT);
    server.info.socket = sock;

    LOGN("Server Started with tick rate %f.", TICK_RATE);

    for(;;)
    {
        for(;;)
        {
            // Read all pending packets
            bool data_waiting = has_data_waiting(server.info.socket);
            if(!data_waiting)
                break;

            Address from = {0};
            Packet recv_pkt = {0};

            bool is_latest;
            int bytes_received = net_recv(&server.info, &from, &recv_pkt, &is_latest);

            if(!validate_packet_format(&recv_pkt))
            {
                LOGN("Invalid packet format!");
                timer_delay_us(1000); // delay 1ms
                continue;
            }

            ClientInfo* cli = NULL;

            bool is_new_client = server_get_client(&from, &cli);

            if(is_new_client)
            {
                if(recv_pkt.hdr.type == PACKET_TYPE_CONNECT_REQUEST)
                {
                    // new client
                    if(recv_pkt.data_len != MAX_PACKET_DATA_SIZE)
                    {
                        LOGN("Packet length doesn't equal %d",MAX_PACKET_DATA_SIZE);
                        remove_client(cli);
                        break;
                    }

                    if(server_assign_new_client(&from, &cli))
                    {
                        cli->state = SENDING_CONNECTION_REQUEST;
                        memcpy(&cli->address,&from,sizeof(Address));
                        update_server_num_clients();

                        LOGN("Welcome New Client! (%d/%d)", server.num_clients, MAX_CLIENTS);
                        print_address(&cli->address);

                        // store salt
                        memcpy(cli->client_salt,&recv_pkt.data[0],8);
                        server_send(PACKET_TYPE_CONNECT_CHALLENGE, cli);
                    }
                    else
                    {
                        // create a temporary ClientInfo so we can send a reject packet back
                        ClientInfo tmp_cli = {0};
                        memcpy(&tmp_cli.address,&from,sizeof(Address));

                        tmp_cli.last_reject_reason = CONNECT_REJECT_REASON_SERVER_FULL;
                        server_send(PACKET_TYPE_CONNECT_REJECTED, &tmp_cli);
                        break;
                    }
                }
            }
            else
            {
                // existing client
                bool auth = authenticate_client(&recv_pkt,cli);

                if(!auth)
                {
                    LOGN("Client Failed authentication");

                    if(recv_pkt.hdr.type == PACKET_TYPE_CONNECT_CHALLENGE_RESP)
                    {
                        cli->last_reject_reason = CONNECT_REJECT_REASON_FAILED_CHALLENGE;
                        server_send(PACKET_TYPE_CONNECT_REJECTED,cli);
                        remove_client(cli);
                    }
                    break;
                }

                switch(recv_pkt.hdr.type)
                {
                    case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
                    {
                        cli->state = SENDING_CHALLENGE_RESPONSE;
                        server_send(PACKET_TYPE_CONNECT_ACCEPTED,cli);
                    } break;
                    case PACKET_TYPE_INPUT:
                    {
                        // apply inputs to player
                        
                    } break;
                    case PACKET_TYPE_DISCONNECT:
                    {
                        remove_client(cli);
                    } break;
                    default:
                    break;
                }
            }

            if(cli != NULL)
            {
                // update client info packet time
                is_latest = is_packet_id_greater(recv_pkt.hdr.id,cli->remote_latest_packet_id);

                if(is_latest)
                {
                    cli->remote_latest_packet_id = recv_pkt.hdr.id;
                    cli->time_of_latest_packet = timer_get_time();
                }
            }


            timer_delay_us(1000); // delay 1ms
        }

        // send state packet to all clients
        if(server.num_clients > 0)
        {
            // disconnect any client that hasn't sent a packet in DISCONNECTION_TIMEOUT
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                ClientInfo* cli = &server.clients[i];

                if(cli == NULL) continue;
                if(cli->state == DISCONNECTED) continue;

                if(cli->time_of_latest_packet > 0)
                {
                    double time_elapsed = timer_get_time() - cli->time_of_latest_packet;

                    if(time_elapsed >= DISCONNECTION_TIMEOUT)
                    {
                        LOGN("Client timed out. Elapsed time: %f", time_elapsed);

                        // disconnect client
                        server_send(PACKET_TYPE_DISCONNECT,cli);
                        remove_client(cli);
                    }
                }
            }

            // send world state to connected clients...
            // @TODO

            //server.info.local_latest_packet_id++;
        }

        timer_wait_for_frame(&server_timer);
    }
}


// =========
// @CLIENT
// =========

struct
{
    Address address;
    NodeInfo info;
    ConnectionState state;
    double time_of_latest_sent_packet;
    double time_of_last_ping;
    uint8_t server_salt[8];
    uint8_t client_salt[8];
    uint8_t xor_salts[8];
    PacketQueue queue;
} client = {0};

typedef struct
{
    double game_time;
    uint16_t input;
} NetPlayerInput;

static NetPlayerInput net_player_inputs[10];
static int input_count;
static const int inputs_per_packet = 2;

bool net_client_add_player_input(uint16_t input, double game_time)
{
    if(input_count >= 10)
    {
        LOGW("Input array is full!");
        return false;
    }

    net_player_inputs[input_count].game_time = game_time;
    memcpy(&net_player_inputs[input_count].input, &input, sizeof(uint16_t));

    input_count++;

    return true;
}

bool net_client_set_server_ip(char* address)
{
    // example input:
    // 200.100.24.10

    char num_str[3] = {0};
    uint8_t   bytes[4]  = {0};

    uint8_t   num_str_index = 0, byte_index = 0;

    for(int i = 0; i < strlen(address)+1; ++i)
    {
        if(address[i] == '.' || address[i] == '\0')
        {
            bytes[byte_index++] = atoi(num_str);
            memset(num_str,0,3*sizeof(char));
            num_str_index = 0;
            continue;
        }

        num_str[num_str_index++] = address[i];
    }

    server.address.a = bytes[0];
    server.address.b = bytes[1];
    server.address.c = bytes[2];
    server.address.d = bytes[3];

    server.address.port = PORT;

    return true;
}

// client information
bool net_client_init()
{
    int sock;

    LOGN("Creating socket.");
    socket_create(&sock);

    client.info.socket = sock;

    packet_queue_create(&client.queue, 32);

    return true;
}

bool net_client_data_waiting()
{
    bool data_waiting = has_data_waiting(client.info.socket);
    return data_waiting;
}

static void client_send(PacketType type)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = client.info.local_latest_packet_id,
        .hdr.ack = client.info.remote_latest_packet_id,
        .hdr.type = type
    };

    switch(type)
    {
        case PACKET_TYPE_CONNECT_REQUEST:
        {
            uint64_t salt = rand64();
            memcpy(client.client_salt, (uint8_t*)&salt,8);

            memset(pkt.data,0,MAX_PACKET_DATA_SIZE);
            memcpy(&pkt.data[0],(uint8_t*)client.client_salt,8);
            pkt.data_len = MAX_PACKET_DATA_SIZE; // pad to 1024

            net_send(&client.info,&server.address,&pkt);

        } break;
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
        {
            store_xor_salts(client.client_salt, client.server_salt, client.xor_salts);

            memset(pkt.data,0,MAX_PACKET_DATA_SIZE);
            memcpy(&pkt.data[0],(uint8_t*)client.xor_salts,8);
            pkt.data_len = MAX_PACKET_DATA_SIZE; // pad to 1024
            net_send(&client.info,&server.address,&pkt);
        } break;
        case PACKET_TYPE_PING:
            memcpy(&pkt.data[0],(uint8_t*)client.xor_salts,8);
            pkt.data_len = 8;
            net_send(&client.info,&server.address,&pkt);
            break;
        case PACKET_TYPE_INPUT:
            memcpy(&pkt.data[0],(uint8_t*)client.xor_salts,8);
            pkt.data[8] = input_count;
            for(int i = 0; i < input_count; ++i)
            {
                int index = 9+(i*sizeof(NetPlayerInput));
                memcpy(&pkt.data[index],&net_player_inputs[i],sizeof(NetPlayerInput));
            }
            pkt.data_len = 9+(input_count*sizeof(NetPlayerInput));
            net_send(&client.info,&server.address,&pkt);
            break;
        case PACKET_TYPE_DISCONNECT:
        {
            pkt.data_len = 0;
            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
                net_send(&client.info,&server.address,&pkt);
        } break;
        default:
            break;
    }

    client.time_of_latest_sent_packet = timer_get_time();
}

bool net_client_connect()
{
    if(client.state != DISCONNECTED)
        return false; // temporary, handle different states in the future

    for(;;)
    {
        client.state = SENDING_CONNECTION_REQUEST;
        client_send(PACKET_TYPE_CONNECT_REQUEST);

        for(;;)
        {
            bool data_waiting = net_client_data_waiting();

            if(!data_waiting)
            {
                double time_elapsed = timer_get_time() - client.time_of_latest_sent_packet;
                if(time_elapsed >= DEFAULT_TIMEOUT)
                    break; // retry sending

                timer_delay_us(1000); // delay 1ms
                continue;
            }

            Packet srvpkt = {0};
            bool is_latest;

            int recv_bytes = net_client_recv(&srvpkt, &is_latest);
            if(recv_bytes > 0)
            {
                switch(srvpkt.hdr.type)
                {
                    case PACKET_TYPE_CONNECT_CHALLENGE:
                    {
                        uint8_t srv_client_salt[8] = {0};
                        memcpy(srv_client_salt, &srvpkt.data[0],8);

                        if(memcmp(srv_client_salt,client.client_salt,8) != 0)
                        {
                            LOGN("Server sent client salt doesn't match actual client salt");
                            return false;
                        }

                        memcpy(client.server_salt,&srvpkt.data[8], 8);
                        LOGN("Received Connect Challenge.");

                        client.state = SENDING_CHALLENGE_RESPONSE;
                        client_send(PACKET_TYPE_CONNECT_CHALLENGE_RESP);
                    } break;
                    case PACKET_TYPE_CONNECT_ACCEPTED:
                    {
                        client.state = CONNECTED;
                        return true;
                    } break;
                    case PACKET_TYPE_CONNECT_REJECTED:
                    {
                        LOGN("Rejection Reason: %s (%02X)", connect_reject_reason_to_str(srvpkt.data[0]), srvpkt.data[0]);
                    } break;
                }
            }

            timer_delay_us(1000); // delay 1000 us
        }
    }
}

void net_client_update()
{
    bool data_waiting = net_client_data_waiting();

    if(data_waiting)
    {
        Packet srvpkt = {0};
        bool is_latest;

        int recv_bytes = net_client_recv(&srvpkt, &is_latest);
        if(recv_bytes > 0)
        {
            switch(srvpkt.hdr.type)
            {
                case PACKET_TYPE_STATE:
                    //@TODO
                    break;
                case PACKET_TYPE_DISCONNECT:
                    client.state = DISCONNECTED;
                    break;
            }
        }
    }

    // handle pinging server
    double time_elapsed = timer_get_time() - client.time_of_last_ping;
    if(time_elapsed >= PING_PERIOD)
    {
        client_send(PACKET_TYPE_PING);
        client.time_of_last_ping = timer_get_time();
    }

    // handle publishing inputs
    if(input_count >= inputs_per_packet)
    {
        client_send(PACKET_TYPE_INPUT);
        input_count = 0;
    }
}

bool net_client_is_connected()
{
    return (client.state == CONNECTED);
}

void net_client_disconnect()
{
    if(client.state != DISCONNECTED)
    {
        client_send(PACKET_TYPE_DISCONNECT);
        client.state = DISCONNECTED;
    }
}

int net_client_send(uint8_t* data, uint32_t len)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = client.info.local_latest_packet_id,
    };

    memcpy(pkt.data,data,len);
    pkt.data_len = len;

    int sent_bytes = net_send(&client.info, &server.address, &pkt);
    return sent_bytes;
}

int net_client_recv(Packet* pkt, bool* is_latest)
{
    Address from = {0};
    int recv_bytes = net_recv(&client.info, &from, pkt, is_latest);
    return recv_bytes;
}

void net_client_deinit()
{
    socket_close(client.info.socket);
}
