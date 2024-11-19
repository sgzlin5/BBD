# BBD

This project is a simple demo to prove it is possible to punch a hole between NAT3 and NAT4 using birthday paradox.

1) Use STUN to test NAT type (easy or hard)
2) Get NATed port range via STUN (10 different port)
3) NAT3 should be easy NAT, NAT4 should be hard NAT
4) NAT4 keep NAT mapping, NAT3 use 600 random ports according to peer's port range to punch a p2p hole

The key point is Client1(NAT3) to predict Client2(NAT4) NATed port

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
make
./bbd -s -l 7777
```

## Edge
```
make
./bbd -c -h host-server.ip -p 7777
```

## Example
```
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 31069
STUN request sent to 49.12.125.53:3479
STUN response received
XOR-Mapped Address: 31088
Local-Deivce Hard-NAT Detected (Local Port [65000] Mapped Port[31069:31088])Client Init Socket OK
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 30958
Get Mapped Port [30958]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 34326
Get Mapped Port [34326]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 33001
Get Mapped Port [33001]
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
Stun receive timeout
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 30996
Get Mapped Port [30996]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 34650
Get Mapped Port [34650]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 11525
Get Mapped Port [11525]
STUN request sent to 49.12.125.53:3478
STUN response received
XOR-Mapped Address: 31106
Get Mapped Port [31106]
Min-Port[11325] Max-Port[34850]
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
We are holder
 => Punch from peer [112.5.155.8:33576] 
nc -u -p 42577 112.5.155.8 33576
 <= Punch
```