# UDP Network Delay Measurement Tool

A C-based UDP application for measuring one-way delay (OWD) and round-trip time (RTT) between two network hosts.

## Features

- Client-server architecture using UDP sockets
- Measures one-way delay (OWD) at the server side
- Measures round-trip time (RTT) at the client side
- Configurable packet size and count
- Configurable inter-packet delay
- Packet loss detection and statistics
- Detailed logging to file

## Build Instructions

Compile both the client and server applications using:

```bash
make
```

This will create two executable files:

- `udp_server`: The server application
- `udp_client`: The client application

## Usage Instructions

### Server

Run the server on the target machine. By default, it listens on port 8888, but you can specify a different port:

```bash
./udp_server [port]
```

For example:

```bash
./udp_server 9000
```

The server will output received packet information and calculate average OWD.

### Client

Run the client on the source machine:

```bash
./udp_client [server_ip] [server_port]
```

For example:

```bash
./udp_client 192.168.1.100 9000
```

If you don't provide IP address or port, the client will prompt you to enter them.

The client will also prompt for:

- Number of packets to send
- Message size in bytes
- Inter-packet delay in seconds

After the test completes, the client will display statistics including:

- Number of packets sent and received
- Packet loss percentage
- Average RTT

## Log Files

Both applications generate log files:

- `server_log.txt`: Contains server-side logs including OWD measurements
- `client_log.txt`: Contains client-side logs including RTT measurements

## Technical Details

- Packet header includes sequence number and timestamp
- Timestamps use microsecond precision
- Network timeouts detect packet loss
- Configurable server port allows testing on different network paths
