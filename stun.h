#ifndef _STUN_H_
#define _STUN_H_

/* Stun */
#define STUN_EASY_NAT       (1)
#define STUN_HARD_NAT       (2)
#define STUN_TEST_PORT      (65432)

#define STUN_SERVER_PORT1           (3478)
#define STUN_SERVER_PORT2           (3479)
#define STUN_MESSAGE_SIZE           (512)

#define STUN_MAGIC_COOKIE           (0x2112A442)
#define STUN_MAGIC_COOKIE_HEAD      (0x2112)
/* STUN Class */
#define STUN_BINDING_REQUEST        (0x0001)
#define STUN_BINDING_RESPONSE       (0x0101)
/* STUN Method */
#define STUN_ATTR_MAPPED_ADDRESS    (0x0001)
#define STUN_ATTR_XMAPPED_ADDRESS   (0x0020)

#define STUN_TRANS_ID_SIZE          (12)

typedef struct {
    uint16_t type;
    uint16_t length;
    uint32_t magic_cookie;
    uint8_t transaction_id[STUN_TRANS_ID_SIZE];
} stun_message_header_t;

int stun_send_binding(uint16_t local_port, char *stun_server, uint16_t server_port, uint16_t *map_port);

#endif /* _STUN_H_ */
