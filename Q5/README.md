# Q5 — Digital library reservation (TCP)

## Purpose

Multi-client **TCP** server (default port **9090**) with **per-connection threads**, mutex-protected **book inventory** and **active user list**, and a simple line-based protocol. Matching **client** prompts for library ID and book index.

## Compilation

```bash
gcc -Wall -Wextra -pthread -O2 -o library_server library_server.c
gcc -Wall -Wextra -O2 -o library_client library_client.c
```

## Execution

### Server (Terminal 1)

```bash
./library_server
```

Expected first line:

```text
Library server listening on port 9090 (max 5 clients)
```

Runs until interrupted (**Ctrl+C**).

### Client (Terminal 2)

```bash
./library_client
```

Optional: pass server IPv4 address (default `127.0.0.1` if you only change host in code — current client uses `argv[1]` for host):

```bash
./library_client 192.168.1.10
```

## Inputs (client, interactive)

1. **Library ID** (must match server allow-list):

   - `LIB1001`
   - `LIB2002`
   - `LIB3003`
   - `LIB4004`
   - `LIB5005`

2. After `AUTH_OK`, **book index** **0 … 5** (integer):

   | Index | Title               |
   | 0     | Systems Programming |
   | 1     | Computer Networks   |
   | 2     | Operating Systems   |
   | 3     | Algorithms          |
   | 4     | Databases           |
   | 5     | Embedded Linux      |

## Expected output (client)

Typical successful session:

```text
Server: HELLO Send AUTH:<library_id>
Enter library ID: LIB1001
Auth result: AUTH_OK
Available books: LIST:...pipe-separated titles...
Enter book index to reserve (0-5): 2
Reservation: RESERVE_OK
Session closed. Goodbye, LIB1001
```

| Server line             | Meaning                                |
|-------------------------|----------------------------------------|
| `AUTH_OK` / `AUTH_FAIL` | Valid / invalid ID or server full      |
| `LIST:...`              | Book titles for authenticated client   |
| `RESERVE_OK`            | Book was available and is now reserved |
| `RESERVE_FAIL`          | Book already reserved                  | 
| `ERR:NOT_AUTH`          | Command needs authentication           |
| `ERR:BAD_BOOK`          | Index out of range                     |
| `ERR:UNKNOWN`           | Unrecognized command                   |

## Expected output (server)

After authentication and reservations, lines like:

```text
[Server] Active users (1): LIB1001
[Server] Books: [0]Systems Programming:available [1]... [2]Operating Systems:reserved ...
```

When a client disconnects, that user is removed from the active list.

## Protocol (summary)

Client sends one line per message (newline-terminated):

- `AUTH:<LIBRARY_ID>`
- `LIST` — requires prior `AUTH_OK`
- `RESERVE:<n>` — integer index; requires auth
- `QUIT`

## Non-interactive quick test

```bash
./library_server &
sleep 0.3
printf 'LIB1001\n2\n' | ./library_client
```

Stop the server afterward.

## Port conflicts

If **9090** is in use, change `PORT` in **both** `library_server.c` and `library_client.c`, recompile.

## Files

| File               | Description                  |
|--------------------|------------------------------|
| `library_server.c` | TCP server, threads, mutexes |
| `library_client.c` | Interactive client           |   
