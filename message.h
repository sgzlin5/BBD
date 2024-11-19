#ifndef _MESSAGE_H_
#define _MESSAGE_H_

int decode_cmd(uint16_t * out, const uint8_t * base, size_t * rem, size_t * idx);
int encode_hello (uint8_t *base, size_t *idx, const struct bbd_hello *reg);
int decode_hello (struct bbd_hello *reg, const uint8_t *base, size_t *rem, size_t *idx);
int encode_peer_info (uint8_t *base, size_t *idx, const struct bbd_peer_info *reg);
int decode_peer_info (struct bbd_peer_info *reg, const uint8_t *base, size_t *rem, size_t *idx);

#endif /* _MESSAGE_H_ */
