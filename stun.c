#include "bbd.h"

/* Stun */
static void stun_generate_transaction_id(uint8_t *transaction_id) {
    for (int i = 0; i < STUN_TRANS_ID_SIZE; i++) {
        transaction_id[i] = rand() % 256;
    }
}

static void stun_parse_response(uint8_t *response, size_t response_length, uint16_t *map_port) {
    if (response_length < sizeof(stun_message_header_t)) {
        printf("Response too short\n");
        return;
    }

    stun_message_header_t *header = (stun_message_header_t *)response;
    header->type = ntohs(header->type);
    header->length = ntohs(header->length);
    header->magic_cookie = ntohl(header->magic_cookie);

    if (header->type != STUN_BINDING_RESPONSE) {
        printf("Not a binding response\n");
        return;
    }

    uint8_t *ptr = response + sizeof(stun_message_header_t);
    size_t remaining_length = response_length - sizeof(stun_message_header_t);

    while (remaining_length > 0) {
        uint16_t attr_type = ntohs(*(uint16_t *)ptr);
        uint16_t attr_length = ntohs(*(uint16_t *)(ptr + 2));
        
        if (attr_length + 4 > remaining_length) {
            printf("Invalid attribute length\n");
            return;
        }

        if (attr_type == STUN_ATTR_MAPPED_ADDRESS) {
            uint16_t port = ((ptr[6] << 8) | ptr[7]);

            printf("Mapped Port:%d\n", port);
            *map_port = port;
            return;
        }else if (attr_type == STUN_ATTR_XMAPPED_ADDRESS) {
            uint16_t port = ((ptr[6] << 8) | ptr[7]) ^ STUN_MAGIC_COOKIE_HEAD; /* Xor with STUN Magic cookie head */

            printf("XOR-Mapped Address: %d\n", port);
            *map_port = port;
            return;
        }

        ptr += 4 + attr_length;
        remaining_length -= (4 + attr_length);
    }
}

int stun_send_binding(uint16_t local_port, char *stun_server, uint16_t server_port, uint16_t *map_port)
{
    int sockfd;
    struct sockaddr_in server_addr, local_addr;
    stun_message_header_t msg;
    uint8_t response[STUN_MESSAGE_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    u_int opt = 1;
    int rc = 0;
    fd_set readfds;
    struct timeval wait_time;
    size_t response_length;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket creation failed\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, stun_server, &server_addr.sin_addr);

    msg.type = htons(STUN_BINDING_REQUEST);
    msg.length = htons(0);
    msg.magic_cookie = htonl(STUN_MAGIC_COOKIE);
    stun_generate_transaction_id(msg.transaction_id);

    memset(&local_addr, 0, sizeof(local_addr));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(socklen_t));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(socklen_t));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd,(struct sockaddr*) &local_addr, sizeof(local_addr)) < 0) {
        printf("bind failed\n");
        close(sockfd);
        return -1;
    }
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    if (sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("sendto failed\n");
        close(sockfd);
        return -1;
    }

    printf("STUN request sent to %s:%d\n", stun_server, server_port);

    wait_time.tv_sec = 1;
    wait_time.tv_usec = 0;
    rc = select(sockfd + 1, &readfds, NULL, NULL, &wait_time);
    if (rc > 0) {
        printf("STUN response received\n");
        response_length = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (response_length < 0) {
            printf("recvfrom failed\n");
            close(sockfd);
            return -1;
        }
    } else {
        printf("Stun receive timeout\n");
        close(sockfd);
        return -1;
    }

    stun_parse_response(response, response_length, map_port);
    close(sockfd);
    return 0;
}