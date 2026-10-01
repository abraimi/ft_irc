# ft\_irc — Project Checklist

> Master checklist for tracking the implementation of the IRC server.

---

# 🖥️ 1. Execution / Networking Side

## Server Setup

* [X]  Create `Server` class
* [X]  Store server port
* [X]  Store server password
* [X]  Create server socket FD
* [X]  Create `_pollfds` vector
* [X]  Create `start()` function
* [X]  Create `setUpSocket()` function
* [X]  Create `run()` function

## Socket Creation

* [X]  Create IPv4 TCP socket with `socket()`
* [X]  Check `socket()` errors
* [X]  Make server socket non-blocking with `fcntl()`
* [X]  Enable `SO_REUSEADDR` with `setsockopt()`
* [X]  Configure `sockaddr_in`
* [X]  Set server port with `htons()`
* [X]  Use `INADDR_ANY`
* [X]  Bind socket with `bind()`
* [X]  Start listening with `listen()`

## Polling System

* [X]  Understand `poll()`
* [X]  Create `pollfd` for server socket
* [X]  Use `POLLIN`
* [X]  Add server FD to `_pollfds`
* [X]  Create main `poll()` loop
* [X]  Check `poll()` return value
* [X]  Check `revents`
* [X]  Detect incoming connections

## Client Connections

* [X]  Accept clients with `accept()`
* [X]  Understand listening FD vs client FD
* [X]  Make client sockets non-blocking
* [X]  Create `pollfd` for each client
* [X]  Add clients to `_pollfds`
* [X]  Monitor multiple clients simultaneously
* [X]  Receive client data with `recv()`
* [X]  Detect client disconnection
* [X]  Close disconnected client sockets
* [X]  Remove disconnected clients from `_pollfds`

## Networking — TODO

* [ ]  Properly handle `POLLERR`
* [ ]  Properly handle `POLLHUP`
* [ ]  Properly handle `POLLNVAL`
* [ ]  Handle `recv()` errors correctly
* [ ]  Handle partial TCP data
* [ ]  Create per-client receive buffers
* [ ]  Aggregate packets until a complete IRC message is received
* [ ]  Handle multiple IRC messages received in one `recv()`
* [ ]  Handle messages split across multiple `recv()` calls
* [ ]  Implement outgoing message buffers
* [ ]  Use `POLLOUT` when necessary
* [ ]  Implement reliable non-blocking `send()`
* [ ]  Handle partial `send()` operations
* [ ]  Implement client cleanup
* [ ]  Implement graceful server shutdown
* [ ]  Handle signals correctly
* [ ]  Make server robust against unexpected client behavior
* [ ]  Test low-bandwidth situations
* [ ]  Test partial packets
* [ ]  Test multiple simultaneous clients
* [ ]  Test client disconnects
* [ ]  Test malformed/network edge cases

---

# 📡 2. Parsing / IRC Protocol Side

## Connection & Authentication

* [ ]  Receive raw IRC messages
* [ ]  Split incoming data into complete IRC commands
* [ ]  Parse IRC command name
* [ ]  Parse command parameters
* [ ]  Handle `PASS`
* [ ]  Verify server password
* [ ]  Handle `NICK`
* [ ]  Validate nickname
* [ ]  Detect duplicate nicknames
* [ ]  Handle `USER`
* [ ]  Store username/user information
* [ ]  Track authentication state
* [ ]  Reject commands from unauthenticated clients when required
* [ ]  Send appropriate IRC responses/errors

## Client/User Management

* [ ]  Create `Client` class
* [ ]  Store client FD
* [ ]  Store nickname
* [ ]  Store username
* [ ]  Store authentication state
* [ ]  Store client information
* [ ]  Manage connected users
* [ ]  Find client by nickname
* [ ]  Detect duplicate nicknames
* [ ]  Remove clients cleanly

## Channels

* [ ]  Create `Channel` class
* [ ]  Store channel name
* [ ]  Store channel topic
* [ ]  Store channel members
* [ ]  Store channel operators
* [ ]  Add client to channel
* [ ]  Remove client from channel
* [ ]  Find channel
* [ ]  Create channel when needed
* [ ]  Remove empty channels if appropriate

## Required IRC Commands

### Basic

* [ ]  `PASS`
* [ ]  `NICK`
* [ ]  `USER`
* [ ]  `JOIN`
* [ ]  `PRIVMSG`

### Channel Operator Commands

* [ ]  `KICK`
* [ ]  `INVITE`
* [ ]  `TOPIC`
* [ ]  `MODE`

## MODE

### Invite-only

* [ ]  `MODE +i`
* [ ]  `MODE -i`

### Topic restriction

* [ ]  `MODE +t`
* [ ]  `MODE -t`

### Channel password

* [ ]  `MODE +k`
* [ ]  `MODE -k`

### Operator privilege

* [ ]  `MODE +o`
* [ ]  `MODE -o`

### User limit

* [ ]  `MODE +l`
* [ ]  `MODE -l`

## Messaging

* [ ]  Private message between two users
* [ ]  Channel messages
* [ ]  Broadcast message to channel members
* [ ]  Don't send channel message back to sender when required
* [ ]  Handle users leaving/disconnecting
* [ ]  Handle messages to nonexistent users
* [ ]  Handle messages to nonexistent channels

## IRC Responses / Errors

* [ ]  Implement required numeric replies
* [ ]  Implement authentication errors
* [ ]  Implement nickname errors
* [ ]  Implement channel errors
* [ ]  Implement permission errors
* [ ]  Implement invalid command handling
* [ ]  Implement missing parameter handling
* [ ]  Implement appropriate IRC error responses

---

# 🔗 3. Execution ↔ Parsing Integration

* [ ]  Connect networking layer to parser
* [ ]  `recv()` → raw IRC data
* [ ]  Raw data → per-client buffer
* [ ]  Buffer → complete IRC command
* [ ]  Complete command → parser
* [ ]  Parser → command handler
* [ ]  Command handler → server/channel/client logic
* [ ]  Generate IRC response
* [ ]  Response → client output buffer
* [ ]  Output buffer → `send()`
* [ ]  Handle partial sends with `POLLOUT`

The final flow should be:

```text
Client
   │
   │ TCP
   ▼
recv()
   │
   ▼
Client Buffer
   │
   ▼
Parser
   │
   ▼
Command Handler
   │
   ├── Client
   ├── Channel
   └── Server
   │
   ▼
Response
   │
   ▼
Output Buffer
   │
   ▼
send()
   │
   ▼
Client
```

---

# 🧪 4. Testing

## Basic Networking

* [X]  Start server
* [X]  Connect with `nc`
* [X]  Receive data from one client
* [X]  Connect multiple clients
* [X]  Receive data from multiple clients
* [X]  Detect client disconnection

## IRC Client

* [ ]  Choose reference IRC client
* [ ]  Connect reference client
* [ ]  Authenticate
* [ ]  Set nickname
* [ ]  Set username
* [ ]  Join channel
* [ ]  Send private message
* [ ]  Receive private message
* [ ]  Send channel message
* [ ]  Receive channel message

## Channel Operator Testing

* [ ]  Test `KICK`
* [ ]  Test `INVITE`
* [ ]  Test `TOPIC`
* [ ]  Test `MODE +i/-i`
* [ ]  Test `MODE +t/-t`
* [ ]  Test `MODE +k/-k`
* [ ]  Test `MODE +o/-o`
* [ ]  Test `MODE +l/-l`

## Edge Cases

* [ ]  Multiple clients connecting simultaneously
* [ ]  Client disconnects unexpectedly
* [ ]  Empty messages
* [ ]  Invalid commands
* [ ]  Missing parameters
* [ ]  Duplicate nickname
* [ ]  Invalid nickname
* [ ]  Invalid channel
* [ ]  Unauthorized channel operations
* [ ]  Sending to nonexistent user
* [ ]  Sending to nonexistent channel
* [ ]  Partial TCP messages
* [ ]  Multiple commands in one packet
* [ ]  Very long messages
* [ ]  Low bandwidth
* [ ]  Client sends data slowly
* [ ]  Client closes connection while server is sending
* [ ]  Server handles many clients without hanging

---

# 🧹 5. Project Quality

* [ ]  C++98 compliant
* [ ]  Compile with `-Wall -Wextra -Werror`
* [ ]  No external libraries
* [ ]  No unnecessary relinking
* [ ]  Makefile: `all`
* [ ]  Makefile: `clean`
* [ ]  Makefile: `fclean`
* [ ]  Makefile: `re`
* [ ]  No memory leaks
* [ ]  No crashes
* [ ]  No blocking I/O
* [ ]  Only one `poll()` / equivalent event loop
* [ ]  Clean client cleanup
* [ ]  Clean channel cleanup
* [ ]  Clean code structure

---

# 📚 6. Documentation

* [ ]  Create `README.md`
* [ ]  Add required 42 first line
* [ ]  Add Description
* [ ]  Add Instructions
* [ ]  Add Resources
* [ ]  Explain how AI was used
* [ ]  Explain compilation
* [ ]  Explain execution
* [ ]  Explain architecture
* [ ]  Explain networking/event loop
* [ ]  Explain IRC protocol implementation
* [ ]  Add testing examples

---

# ⭐ 7. Bonus — Only After Mandatory Is Perfect

* [ ]  File transfer
* [ ]  IRC bot

> **Important:** Do not start bonus work until every mandatory requirement works correctly.

---

# 📍 Current Progress

### Execution / Networking

**Completed:** Socket creation → non-blocking → bind → listen → poll → accept → multiple clients → recv → disconnect cleanup.

### Parsing / IRC

**Current status:** Being handled by my peer.

### Next Networking Goal

**Implement robust TCP message buffering and connect the networking layer to the parser.**
