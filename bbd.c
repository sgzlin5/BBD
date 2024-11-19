#include "bbd.h"

char* intoa (uint32_t /* host order */ addr, char* buf, uint16_t buf_len) {

    char *cp, *retStr;
    uint8_t byteval;
    int n;

    cp = &buf[buf_len];
    *--cp = '\0';

    n = 4;
    do {
        byteval = addr & 0xff;
        *--cp = byteval % 10 + '0';
        byteval /= 10;
        if(byteval > 0) {
            *--cp = byteval % 10 + '0';
            byteval /= 10;
            if(byteval > 0) {
                *--cp = byteval + '0';
            }
        }
        *--cp = '.';
        addr >>= 8;
    } while(--n > 0);

    /* Convert the string to lowercase */
    retStr = (char*)(cp + 1);

    return(retStr);
}

static void help (int level){
    if (level == 0) {
        return;
    }
    printf("\n");
    printf("   usage: \n"
        "\n *** Server ***"
        "\n -s <work in server mode>"
        "\n -l <server listening port>"
        "\n *** Client ***"
        "\n -c <work in client mode>"
        "\n -h <server ip>"
        "\n -p <server port>"
        "\n\n");
    exit(0);
}

/* bbd -s -l 7777*/
/* bbd -c -h shinya.icu -p 7777 */
static int setOption (int optkey, char *optargument, struct bbd *bbd)
{
    switch(optkey) {
        case 's': {
            bbd->mode |= bbd_server;
            break;
        }
        case 'c': {
            bbd->mode |= bbd_client;
            break;
        }
        case 'l': {
            bbd->sss.server_listen_port = atoi(optargument);
            break;
        }
        case 'h': {
            memcpy(bbd->ccc.server_ip, optargument, INET_ADDRSTRLEN);
            break;
        }
        case 'p': {
            bbd->ccc.server_port = atoi(optargument);
            break;
        }
        default: {
            printf("unknown option -%c\n", (char)optkey);
            return 2;
        }
    }
    return 0;
}

static void bbd_loadcli(int argc, char* argv[], struct bbd *bbd)
{
    u_char c;

    while((c = getopt_long(argc, argv,
            "scl:h:p:", NULL, NULL)) != '?') {
        
        if(c == 255) break;
        help(setOption(c, optarg, bbd));
    }
    if (bbd->mode >= bbd_mode_max || bbd->mode <= bbd_mode_min) {
        help(1);
    }

    return;
}

/* Server */
static void bbd_server_close_socket(struct bbd *bbd)
{
    close(bbd->sss.sock);
}

static int bbd_server_init_socket(struct bbd *bbd)
{
    u_int opt = 1;

    /* Server UDP socket */
    bbd->sss.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bbd->sss.sock < 0) {
        bbd_server_close_socket(bbd);
        printf("UDP socket init failed\n");
        return -1;
    }
    setsockopt(bbd->sss.sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(socklen_t));
    setsockopt(bbd->sss.sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(socklen_t));
    memset(&bbd->sss.saddr, 0, sizeof(bbd->sss.saddr));
    bbd->sss.saddr.sin_family = AF_INET;
    bbd->sss.saddr.sin_port = htons(bbd->sss.server_listen_port);
    bbd->sss.saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(bbd->sss.sock,(struct sockaddr*) &bbd->sss.saddr, sizeof(bbd->sss.saddr)) == -1) {
        bbd_server_close_socket(bbd);
        printf("UDP socket bind failed\n");
        return -1;
    }
    printf("Server socket init OK\n");
    return 0;
}

static int bbd_server_wait_client(struct bbd *bbd)
{
    struct sockaddr_in sender_sock;
    socklen_t slen;
    int tmp_sock, rc, max_sd;
    fd_set socket_mask;
    struct timeval wait_time;
    ssize_t recvlen;
    char buf[BBD_PKT_BUF_SIZE];
    uint16_t cmd;
    struct bbd_hello hello;
    size_t idx, rem;

    memset(buf, 0, BBD_PKT_BUF_SIZE);
    FD_ZERO(&socket_mask);
    FD_SET(bbd->sss.sock, &socket_mask);
    max_sd = bbd->sss.sock;
    wait_time.tv_sec = 60;
    wait_time.tv_usec = 0;
    slen = sizeof(sender_sock);
    while (bbd->sss.conn_num < 2) {
        rc = select(max_sd + 1, &socket_mask, NULL, NULL, &wait_time);
        if (rc > 0) {
            if (FD_ISSET(bbd->sss.sock, &socket_mask)) {
                recvlen = recvfrom(bbd->sss.sock, buf, BBD_PKT_BUF_SIZE, 0/*flags*/,
                    (struct sockaddr *) &sender_sock, (socklen_t *) &slen);
                printf("Receive [%s] Length [%d]\n", buf, recvlen);
                rem = recvlen;
                idx = 0;
                decode_cmd(&cmd, buf, &rem, &idx);
                if (cmd == bbd_msg_hello) {
                    memset(&hello, 0, sizeof(struct bbd_hello));
                    printf(" <= HELLO from Client-%d\n", bbd->sss.conn_num);
                    memcpy(&(bbd->sss.conn_addr[bbd->sss.conn_num]), &sender_sock, sizeof(struct sockaddr_in));
                    decode_hello(&hello, buf, &rem, &idx);
                    sendto(bbd->sss.sock, buf, recvlen, 0, (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in));
                    bbd->sss.nat_type[bbd->sss.conn_num] = hello.nat_type;
                    bbd->sss.min_port[bbd->sss.conn_num] = hello.min_port;
                    bbd->sss.max_port[bbd->sss.conn_num] = hello.max_port;
                    printf(" <= NAT:%d Min:%d Max:%d\n", bbd->sss.nat_type[bbd->sss.conn_num], bbd->sss.min_port[bbd->sss.conn_num], bbd->sss.max_port[bbd->sss.conn_num]);
                    bbd->sss.conn_num++;
                }
            }
        }
    }
    return 0;
}

static int bbd_server_start(struct bbd *bbd)
{
    int ret;
    int ok1, ok2;
    fd_set readfds;
    char buf[BBD_PKT_BUF_SIZE];
    char peer_ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in sender_sock;
    socklen_t slen;
    ssize_t recvlen;
    uint16_t cmd;
    struct bbd_hello hello;
    size_t idx, rem;
    struct timeval wait_time;

    slen = sizeof(sender_sock);
    bbd_server_init_socket(bbd);
    bbd_server_wait_client(bbd);
    // /* Ack to clients */
    // sendto(bbd->sss.sock, "HACK", strlen("HACK"), 0, (struct sockaddr *)&bbd->sss.conn_addr[0], sizeof(struct sockaddr_in));
    // sendto(bbd->sss.sock, "HACK", strlen("HACK"), 0, (struct sockaddr *)&bbd->sss.conn_addr[1], sizeof(struct sockaddr_in));

    while (1) {
        wait_time.tv_sec = 10;
        wait_time.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(bbd->sss.sock, &readfds);
        int rc = select(bbd->sss.sock + 1, &readfds, NULL, NULL, &wait_time);
        if (rc > 0 && FD_ISSET(bbd->sss.sock, &readfds)) {
            recvlen = recvfrom(bbd->sss.sock, buf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&sender_sock, &slen);
            if (recvlen <= 0) {
                return -1;
            }
            memset(peer_ip_str, 0, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &sender_sock.sin_addr, peer_ip_str, sizeof(peer_ip_str));
            printf("Receive UDP from Peer %s:%d\n", peer_ip_str, ntohs(sender_sock.sin_port));
            rem = recvlen;
            idx = 0;
            decode_cmd(&cmd, buf, &rem, &idx);
            if (cmd == bbd_msg_peer_info) {
                struct bbd_peer_info info[2];
                char ptkbuf[BBD_PKT_BUF_SIZE];
                char ptkbuf2[BBD_PKT_BUF_SIZE];
                size_t id = 0, id2 = 0;
                memset(&info[0], 0, sizeof(struct bbd_peer_info));
                memset(&info[1], 0, sizeof(struct bbd_peer_info));
                memset(ptkbuf, 0, BBD_PKT_BUF_SIZE);
                memset(ptkbuf2, 0, BBD_PKT_BUF_SIZE);

                if (bbd->sss.nat_type[0] != bbd->sss.nat_type[1]) {
                    if (bbd->sss.nat_type[0] > bbd->sss.nat_type[1]) {
                        info[0].bbd_type = BBD_HOLDER;
                        info[1].bbd_type = BBD_VISITOR;
                    } else {
                        info[0].bbd_type = BBD_VISITOR;
                        info[1].bbd_type = BBD_HOLDER;
                    }
                } else {
                    if ((bbd->sss.max_port[0] - bbd->sss.min_port[0]) > (bbd->sss.max_port[1] - bbd->sss.min_port[1])) {
                        info[0].bbd_type = BBD_HOLDER;
                        info[1].bbd_type = BBD_VISITOR;
                    } else {
                        info[0].bbd_type = BBD_VISITOR;
                        info[1].bbd_type = BBD_HOLDER;
                    }
                }
                info[0].cmd = bbd_msg_peer_info;
                info[0].peer_min_port = bbd->sss.min_port[1];
                info[0].peer_max_port = bbd->sss.max_port[1];
                info[0].peer_port = ntohs(bbd->sss.conn_addr[1].sin_port);
                info[0].peer_ip = bbd->sss.conn_addr[1].sin_addr.s_addr;
                id = 0;
                encode_peer_info(ptkbuf, &id, &info[0]);
                info[1].cmd = bbd_msg_peer_info;
                info[1].peer_min_port = bbd->sss.min_port[0];
                info[1].peer_max_port = bbd->sss.max_port[0];
                info[1].peer_port = ntohs(bbd->sss.conn_addr[0].sin_port);
                info[1].peer_ip = bbd->sss.conn_addr[0].sin_addr.s_addr;
                id2 = 0;
                encode_peer_info(ptkbuf2, &id2, &info[1]);

                sendto(bbd->sss.sock, ptkbuf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&bbd->sss.conn_addr[0], sizeof(struct sockaddr_in));
                sendto(bbd->sss.sock, ptkbuf2, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&bbd->sss.conn_addr[1], sizeof(struct sockaddr_in));
                break;
            }
        }
    }

    bbd_server_close_socket(bbd);
    bbd->sss.conn_num = 0;
    return 0;
}

static uint16_t bbd_get_port_from_socket(int sock)
{
    uint16_t port;
    struct sockaddr_in tmp_addr;
    socklen_t addr_len = sizeof(tmp_addr);

    memset(&tmp_addr, 0, sizeof(struct sockaddr_in));
    getsockname(sock, (struct sockaddr *)&tmp_addr, &addr_len);
    port = ntohs(tmp_addr.sin_port);
    return port;
}

static int bbd_client_init_socket(struct bbd *bbd)
{
    struct sockaddr_in peer_addr;
    u_int opt = 1;

    memset(&bbd->ccc.server_addr, 0, sizeof(bbd->ccc.server_addr));
    bbd->ccc.server_addr.sin_family = AF_INET;
    bbd->ccc.server_addr.sin_port = htons(bbd->ccc.server_port);
    inet_pton(AF_INET, bbd->ccc.server_ip, &bbd->ccc.server_addr.sin_addr);

    memset(&peer_addr, 0, sizeof(peer_addr));
    bbd->ccc.punch_pool[0] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(bbd->ccc.punch_pool[0], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(socklen_t));
    setsockopt(bbd->ccc.punch_pool[0], SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(socklen_t));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(0);
    peer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(bbd->ccc.punch_pool[0],(struct sockaddr*) &peer_addr, sizeof(peer_addr));
    bbd->ccc.punch_port = bbd_get_port_from_socket(bbd->ccc.punch_pool[0]);
    for (int i = 1; i < BBD_MAX_SOCKET; i++) {
        bbd->ccc.punch_pool[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        setsockopt(bbd->ccc.punch_pool[i], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(socklen_t));
        setsockopt(bbd->ccc.punch_pool[i], SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(socklen_t));
        memset(&peer_addr, 0, sizeof(peer_addr));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(bbd->ccc.punch_port + i);
        peer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind(bbd->ccc.punch_pool[i],(struct sockaddr*) &peer_addr, sizeof(peer_addr));
    }
    printf("Client Init Socket OK\n");
    return 0;
}

static void nat_type_test(struct bbd *bbd)
{
    uint16_t map_port[2] = {0};
    uint16_t local_port = 65000;
    int ret;
    int cnt = 0, type;

    type = 0;
    while (1) {
first_stun:
        ret = stun_send_binding(local_port, "49.12.125.53", 3478, &map_port[0]);
        usleep(10000);
        if (ret < 0 && cnt < 5) {
            cnt++;
            goto first_stun;
        }
second_stun:
        ret = stun_send_binding(local_port, "49.12.125.53", 3479, &map_port[1]);
        usleep(10000);
        if (ret < 0 && cnt < 5) {
            cnt++;
            goto second_stun;
        }
        break;
    }

    if (map_port[0] == map_port[1] && map_port[0] != 0 && map_port[1] != 0) {
        bbd->ccc.nat_type = BBD_EASY_NAT;
    } else {
        bbd->ccc.nat_type = BBD_HARD_NAT;
    }
    printf("Local-Deivce %s-NAT Detected (Local Port [%d] Mapped Port[%d:%d])", 
        (bbd->ccc.nat_type == BBD_EASY_NAT) ? "Easy" : "Hard", local_port, map_port[0], map_port[1]);
}

static int bbd_generate_ports(uint16_t *ports, uint16_t max, uint16_t min)
{
    int port;
    int exists = 0;
    int count = 0;
    int max_range = BBD_PORT_COUNT;

    if ((max - min) < max_range) {
        max_range = (max - min);
    }
    for(int i = 0; i < max_range; i++) {
        ports[i]=0;
    }
    srand(time(NULL));
    while (count < max_range) {
        port = rand() % (max - min + 1) + min;
        exists = 0;
        for (int i = 0; i < count; i++) {
            if (ports[i] == port) {
                exists = 1;
                break;
            }
        }

        if (!exists) {
            ports[count] = port;
            count++;
        }
    }
    return max_range;
}

static void bbd_get_port_range(struct bbd *bbd)
{
    uint16_t ports = 0;
    int ret;
    uint16_t local_port = bbd->ccc.punch_port;

    bbd->ccc.min_port = 0xffff;
    bbd->ccc.max_port = 0;
    for(int i = 0; i < 10; i++) {
        ports = 0;
        ret = stun_send_binding(local_port + i, "49.12.125.53", 3478, &ports);
        if (ret == 0) {
            printf("Get Mapped Port [%d]\n", ports);
            bbd->ccc.min_port = (ports < bbd->ccc.min_port) ? ports : bbd->ccc.min_port;
            bbd->ccc.max_port = (bbd->ccc.max_port < ports) ? ports : bbd->ccc.max_port;
        }
        usleep(50000);
    }
    bbd->ccc.min_port = ((bbd->ccc.min_port - 200) > 0) ? (bbd->ccc.min_port - 200) : bbd->ccc.min_port;
    bbd->ccc.max_port = ((bbd->ccc.max_port + 200) < 0xffff) ? (bbd->ccc.max_port + 200) : bbd->ccc.max_port;
    printf("Min-Port[%d] Max-Port[%d]\n", bbd->ccc.min_port, bbd->ccc.max_port);
}

static int bbd_client_punch(struct bbd *bbd)
{
    char buf[BBD_PKT_BUF_SIZE];
    struct sockaddr_in sender_sock;
    socklen_t slen;
    char revbuf[BBD_PKT_BUF_SIZE];
    int pool_id = 0;
    size_t idx, rem;;
    ssize_t recvlen;
    char ptkbuf[BBD_PKT_BUF_SIZE];
    int max_sd = 0;
    int rc = 0;
    fd_set readfds;
    struct timeval wait_time;
    struct bbd_hello hello;
    int cnt = 0;
    int cmd;

    nat_type_test(bbd);
    bbd_client_init_socket(bbd);

    bbd_get_port_range(bbd);

retry_hello:
    /* Send hello to server */
    hello.cmd = bbd_msg_hello;
    hello.nat_type = BBD_HARD_NAT;
    hello.min_port = bbd->ccc.min_port;
    hello.max_port = bbd->ccc.max_port;
    idx = 0;
    memset(ptkbuf, 0, BBD_PKT_BUF_SIZE);
    encode_hello(ptkbuf, &idx, &hello);
    sendto(bbd->ccc.punch_pool[0], ptkbuf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&bbd->ccc.server_addr, sizeof(struct sockaddr_in));
    printf(" <= Hello \n");

    /* Wait for hello ack */
    FD_ZERO(&readfds);
    FD_SET(bbd->ccc.punch_pool[0], &readfds);
    slen = sizeof(sender_sock);
    memset(buf, 0, BBD_PKT_BUF_SIZE);
    wait_time.tv_sec = 10;
    wait_time.tv_usec = 0;
    max_sd = bbd->ccc.punch_pool[0];
    rc = select(max_sd + 1, &readfds, NULL, NULL, &wait_time);
    if (rc > 0) {
        recvlen = recvfrom(bbd->ccc.punch_pool[0], buf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&sender_sock, &slen);
        if(recvlen < 0) {
            goto retry_hello;
        }
        rem = recvlen;
        idx = 0;
        decode_cmd(&cmd, buf, &rem, &idx);
        if (cmd != bbd_msg_hello) {
            goto retry_hello;
        }
    } else {
        goto retry_hello;
    }
retry_peer:
    printf(" => Ask Peer Info \n");
    struct bbd_peer_info info;
    memset(&info, 0, sizeof(struct bbd_peer_info));
    info.cmd = bbd_msg_peer_info;
    idx = 0;
    memset(ptkbuf, 0, BBD_PKT_BUF_SIZE);
    encode_hello(ptkbuf, &idx, &info);
    sendto(bbd->ccc.punch_pool[0], ptkbuf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&bbd->ccc.server_addr, sizeof(struct sockaddr_in));
    sendto(bbd->ccc.punch_pool[0], ptkbuf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&bbd->ccc.server_addr, sizeof(struct sockaddr_in));

    FD_ZERO(&readfds);
    FD_SET(bbd->ccc.punch_pool[0], &readfds);
    slen = sizeof(sender_sock);
    memset(buf, 0, BBD_PKT_BUF_SIZE);
    wait_time.tv_sec = 10;
    wait_time.tv_usec = 0;
    max_sd = bbd->ccc.punch_pool[0];
    rc = select(max_sd + 1, &readfds, NULL, NULL, &wait_time);
    if (rc > 0) {
        recvlen = recvfrom(bbd->ccc.punch_pool[0], buf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&sender_sock, &slen);
        if(recvlen < 0) {
            goto retry_peer;
        }
        rem = recvlen;
        idx = 0;
        decode_cmd(&cmd, buf, &rem, &idx);
        if (cmd != bbd_msg_peer_info) {
            goto retry_peer;
        } else {
            printf(" <= Receive Peer Info\n");
            memset(&info, 0, sizeof(struct bbd_peer_info));
            decode_peer_info(&info, buf, &rem, &idx);
            bbd->ccc.bbd_role = info.bbd_type;
            bbd->ccc.peer_min_port = info.peer_min_port;
            bbd->ccc.peer_max_port = info.peer_max_port;
            memset(&bbd->ccc.punch_addr, 0, sizeof(bbd->ccc.punch_addr));
            bbd->ccc.punch_addr.sin_family = AF_INET;
            bbd->ccc.punch_addr.sin_port = htons(info.peer_port);
            bbd->ccc.punch_addr.sin_addr.s_addr = info.peer_ip;
        }
    } else {
        goto retry_peer;
    }
again:
    FD_ZERO(&readfds);
    wait_time.tv_sec = 3;
    wait_time.tv_usec = 0;
    max_sd = 0;
    switch(bbd->ccc.bbd_role) {
        case BBD_HOLDER: {
            printf("We are holder\n");
            for (int id = 0; id < BBD_MAX_SOCKET; id++) {
                if (max_sd < bbd->ccc.punch_pool[id]) {
                    max_sd = bbd->ccc.punch_pool[id];
                }
                FD_SET(bbd->ccc.punch_pool[id], &readfds);
                sendto(bbd->ccc.punch_pool[id], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&(bbd->ccc.punch_addr), sizeof(struct sockaddr_in));
                sendto(bbd->ccc.punch_pool[id], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&(bbd->ccc.punch_addr), sizeof(struct sockaddr_in));
                sendto(bbd->ccc.punch_pool[id], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&(bbd->ccc.punch_addr), sizeof(struct sockaddr_in));
            }
            break;
        }
        case BBD_VISITOR: {
            
            uint16_t ports[BBD_PORT_COUNT];
            int max_range;
            max_sd = bbd->ccc.punch_pool[0];
            FD_SET(bbd->ccc.punch_pool[0], &readfds);
            max_range = bbd_generate_ports(ports, bbd->ccc.peer_max_port, bbd->ccc.peer_min_port);
            printf("We are visitor, peer range[%d ~ %d]\n", bbd->ccc.peer_max_port, bbd->ccc.peer_min_port);
            for (int k = 0; k < max_range; k++) {
                bbd->ccc.punch_addr.sin_port = htons(ports[k]);
                sendto(bbd->ccc.punch_pool[0], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&(bbd->ccc.punch_addr), sizeof(struct sockaddr_in));
                sendto(bbd->ccc.punch_pool[0], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&(bbd->ccc.punch_addr), sizeof(struct sockaddr_in));
                sendto(bbd->ccc.punch_pool[0], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&(bbd->ccc.punch_addr), sizeof(struct sockaddr_in));
            }
            break;
        }
    }

    slen = sizeof(sender_sock);
    memset(revbuf, 0, BBD_PKT_BUF_SIZE);
    char tmp[32];
    char peer_ip_str[INET_ADDRSTRLEN];
    rc = select(max_sd + 1, &readfds, NULL, NULL, &wait_time);
    if (rc > 0) {
        for (int id = 0; id < BBD_MAX_SOCKET; id++) {
            if (FD_ISSET(bbd->ccc.punch_pool[id], &readfds)) {
                char tmp_buf[BBD_PKT_BUF_SIZE];
                pool_id = id;
                recvfrom(bbd->ccc.punch_pool[pool_id], revbuf, BBD_PKT_BUF_SIZE, 0, (struct sockaddr *)&sender_sock, &slen);
                if (!strncmp(BBD_PUNCH_MSG, revbuf, strlen(BBD_PUNCH_MSG))) {
                    printf(" => Punch from peer [%s:%d] \n", intoa(ntohl(sender_sock.sin_addr.s_addr), tmp, sizeof(tmp)), ntohs(sender_sock.sin_port));
                    printf("nc -u -p %d %s %d\n", bbd_get_port_from_socket(bbd->ccc.punch_pool[pool_id]), intoa(ntohl(sender_sock.sin_addr.s_addr), tmp, sizeof(tmp)), ntohs(sender_sock.sin_port));
                } else {
                    if (cnt < 10) {
                        goto again;
                    } else {
                        return -1;
                    }
                }
            }
        }
    } else {
        if (cnt < 10) {
            cnt++;
            goto again;
        } else {
            return -1;
        }
    }

    for (int i = 0; i < 10 ; i++) {
        sendto(bbd->ccc.punch_pool[pool_id], BBD_PUNCH_MSG, strlen(BBD_PUNCH_MSG), 0, (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in));
    }
    printf(" <= Punch \n");

    for (int id = 0; id < BBD_MAX_SOCKET; id++) {
        close(bbd->ccc.punch_pool[id]);
    }
    return 0;
}

static int bbd_client_start(struct bbd *bbd)
{
    while(1) {
        if (bbd_client_punch(bbd) == 0) {
            break;
        }
    }
    return 0;
}

static void bbd_start(struct bbd *bbd)
{
    int ret;

    if (bbd->mode == bbd_server) {
        while (1) {
            bbd_server_start(bbd);
        }
    } else if (bbd->mode == bbd_client) {
        bbd_client_start(bbd);
    } else {
        help(1);
    }
}

int main (int argc, char* argv[])
{
    struct bbd bbd;

    memset(&bbd, 0, sizeof(struct bbd));
    bbd_loadcli(argc, argv, &bbd);
    bbd_start(&bbd);

    return 0;
}