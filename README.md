# BBD

This project is a simple demo to prove it is possible to punch a hole between NAT3 and NAT4 using birthday paradox.

1) Use STUN to test NAT type (easy or hard)
2) Get NATed port range via STUN (10 different port)
3) NAT3 should be easy NAT, NAT4 should be hard NAT
4) NAT4 keep NAT mapping, NAT3 use 600 random ports according to peer's port range to punch a p2p hole

```
Process:

Client ---    Hello     ---> Server
Client <---   Hello     ---- Server
Client ---   Peer Info  ---> Server
Client <--   Peer Info  ---> Server

Client1:P1[0:24]  ---   Punch Msg  ---> Client2:P2 (Hold NAT mapping)

Client2:P2        ---   Punch Msg  ---> Client1:P3[0:599] (Perdict peer's port)

```
## Host
```
cmake .
make
./bbd -s -l 7777
```

## Edge
```
cmake .
make
./bbd -c -h host-server.ip -p 7777
```

## Example
```
Client A:

STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 33191
STUN request sent to 49.12.125.53:3479
STUN response received
XOR-Mapped Address: 33208
Local-Deivce Hard-NAT Detected (Local Port [65000] Mapped Port[33191:33208])Client Init Socket OK
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 30869
Get Mapped Port [30869]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 34318
Get Mapped Port [34318]
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 33218
Get Mapped Port [33218]
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
Min-Port[30669] Max-Port[34518]
 <= Hello 
 => Ask Peer Info 
 <= Receive Peer Info
We are holder
We are holder
We are holder
We are holder
We are holder
We are holder
We are holder
We are holder
 => Punch from peer [112.5.155.8:40492] 
nc -u -p 43403 112.5.155.8 40492
 <= Punch 
```

```
Client B:

STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 65000
STUN request sent to 49.12.125.53:3479
STUN response received
XOR-Mapped Address: 65000
Local-Deivce Easy-NAT Detected (Local Port [65000] Mapped Port[65000:65000])Client Init Socket OK
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 40494
Get Mapped Port [40494]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 40495
Get Mapped Port [40495]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 40496
Get Mapped Port [40496]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 40497
Get Mapped Port [40497]
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 40501
Get Mapped Port [40501]
Min-Port[40294] Max-Port[40701]
 <= Hello 
 => Ask Peer Info 
 <= Receive Peer Info
We are visitor, peer range[34518 ~ 30669]
 => Punch from peer [112.49.223.217:34438] 
nc -u -p 40492 112.49.223.217 34438
 <= Punch
```