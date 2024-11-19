#ifndef _BBD_H_
#define _BBD_H_

#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include "stun.h"

/*
    1. Test NAT Behavior (Easy or Hard)
    2. Test NAT Port (local: 60000~60009 --> remove: min-port~max-port eg. 7890~12345) 
    3. Send Nat-Behavior and Port-range to Server
    4. Server set one as holder and another as visitor, then send start punching
    5. Holder just keep NAT mapping
    6. Visitor generate port randomly according to peer's port range
*/
typedef enum bbd_message {
    bbd_msg_hello = 0x01,
    bbd_msg_peer_info = 0x02
} bbd_message_t;

#define IS_VALID_MSG(cmd) (cmd == bbd_msg_hello) || (cmd == bbd_msg_peer_info)

#define BBD_PKT_BUF_SIZE         (512)
#define BBD_PUNCH_MSG            "IsBBDPunch"
#define BBD_MAX_SOCKET           (25)
#define BBD_PORT_COUNT           (600)
/* NAT Behavior type */
#define BBD_EASY_NAT             (1)
#define BBD_HARD_NAT             (2)
/* BBD Character */
#define BBD_HOLDER               (0x0a)
#define BBD_VISITOR              (0x05)

typedef enum bbd_mode {
    bbd_mode_min = 0,
    bbd_server = 1,
    bbd_client,
    bbd_mode_max
} bbd_mode_t;

struct bbd_client {
    char                server_ip[INET_ADDRSTRLEN];
    uint16_t            server_port;
    struct sockaddr_in  server_addr;
    int                 sock;   /* Sock to connect with server */
    uint16_t            punch_pool[BBD_MAX_SOCKET];
    uint16_t            punch_port;
    struct sockaddr_in  punch_addr;
    uint16_t            peer_min_port;
    uint16_t            peer_max_port;
    int                 nat_type;
    uint16_t            bbd_role;
    uint16_t            min_port;
    uint16_t            max_port;
};

struct bbd_server {
    int                 sock;
    struct sockaddr_in  saddr;
    uint16_t            server_listen_port;
    int                 conn_num;
    int                 conn_sock[2];
    struct sockaddr_in  conn_addr[2];
    int                 nat_type[2];
    int                 min_port[2];
    int                 max_port[2];
    uint16_t            role[2];
};

struct bbd {
    /* Common */
    int mode;           /* Run as server or client */
    struct bbd_server sss;
    struct bbd_client ccc;
};

struct bbd_hello {
    uint16_t cmd;
    uint32_t nat_type;
    uint16_t min_port;
    uint16_t max_port;
};

struct bbd_peer_info {
    uint16_t cmd;
    uint16_t bbd_type;
    uint16_t peer_min_port;
    uint16_t peer_max_port;
    uint16_t peer_port;
    uint32_t peer_ip;
};

#endif /* _BBD_H_ */