#include "bbd.h"

/* Common */
static int encode_uint16 (uint8_t * base,
                   size_t * idx,
                   const uint16_t v) {

    *(base + (*idx))     = ( v >> 8) & 0xff;
    *(base + (1 + *idx)) = ( v & 0xff );
    *idx += 2;

    return 2;
}

static int decode_uint16 (uint16_t * out,
                   const uint8_t * base,
                   size_t * rem,
                   size_t * idx) {

    if(*rem < 2) {
        return 0;
    }

    *out  = ( base[*idx] & 0xff ) << 8;
    *out |= ( base[1 + *idx] & 0xff );
    *idx += 2;
    *rem -= 2;

    return 2;
}

static int encode_uint32 (uint8_t * base,
                   size_t * idx,
                   const uint32_t v) {

    *(base + (0 + *idx)) = ( v >> 24) & 0xff;
    *(base + (1 + *idx)) = ( v >> 16) & 0xff;
    *(base + (2 + *idx)) = ( v >> 8) & 0xff;
    *(base + (3 + *idx)) = ( v & 0xff );
    *idx += 4;

    return 4;
}

static int decode_uint32 (uint32_t * out,
                   const uint8_t * base,
                   size_t * rem,
                   size_t * idx) {

    if(*rem < 4) {
        return 0;
    }

    *out  = ( base[0 + *idx] & 0xff ) << 24;
    *out |= ( base[1 + *idx] & 0xff ) << 16;
    *out |= ( base[2 + *idx] & 0xff ) << 8;
    *out |= ( base[3 + *idx] & 0xff );
    *idx += 4;
    *rem -= 4;

    return 4;
}

int decode_cmd(uint16_t * out, const uint8_t * base, size_t * rem, size_t * idx)
{

    size_t idx0 = *idx;
    uint16_t dummy = 0;

    decode_uint16(&dummy, base, rem, idx);
    *out = dummy;
    return (*idx - idx0);
}

int encode_hello(uint8_t *base, size_t *idx, const struct bbd_hello *reg)
{
    int retval = 0;

    retval += encode_uint16(base, idx, reg->cmd);
    retval += encode_uint32(base, idx, reg->nat_type);
    retval += encode_uint16(base, idx, reg->min_port);
    retval += encode_uint16(base, idx, reg->max_port);
    return retval;
}

int decode_hello(struct bbd_hello *reg, const uint8_t *base, size_t *rem, size_t *idx)
{

    size_t retval = 0;
    memset(reg, 0, sizeof(struct bbd_hello));

    retval += decode_uint32(&(reg->nat_type), base, rem, idx);
    retval += decode_uint16(&(reg->min_port), base, rem, idx);
    retval += decode_uint16(&(reg->max_port), base, rem, idx);
    return retval;
}

int encode_peer_info(uint8_t *base, size_t *idx, const struct bbd_peer_info *reg)
{

    int retval = 0;

    retval += encode_uint16(base, idx, reg->cmd);
    retval += encode_uint16(base, idx, reg->bbd_type);
    retval += encode_uint16(base, idx, reg->peer_min_port);
    retval += encode_uint16(base, idx, reg->peer_max_port);
    retval += encode_uint16(base, idx, reg->peer_port);
    retval += encode_uint32(base, idx, reg->peer_ip);
    return retval;
}

int decode_peer_info(struct bbd_peer_info *reg, const uint8_t *base, size_t *rem, size_t *idx)
{

    size_t retval = 0;
    memset(reg, 0, sizeof(struct bbd_peer_info));

    retval += decode_uint16(&(reg->bbd_type), base, rem, idx);
    retval += decode_uint16(&(reg->peer_min_port), base, rem, idx);
    retval += decode_uint16(&(reg->peer_max_port), base, rem, idx);
    retval += decode_uint16(&(reg->peer_port), base, rem, idx);
    retval += decode_uint32(&(reg->peer_ip), base, rem, idx);
    return retval;
}