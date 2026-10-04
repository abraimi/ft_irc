# IRC Server Project Guide

This document is a standalone brief for an AI coding agent that needs to understand, implement, or rebuild this project. It describes the intended server, the current source layout, each class's responsibility, the protocol flow, and a practical implementation order.

## 1. Project goal

Build a small IRC server named `ircserv`. It accepts a TCP port and connection password, handles multiple clients concurrently with `poll`, registers users, manages channels, and relays IRC commands and messages. It is a single server process; it does not connect to other IRC servers.

The current project is written in C++98 and uses POSIX sockets. Preserve that baseline unless the project owner explicitly changes it. The server is launched as:

```sh
make
./ircserv <port> <password>
```

The executable's usage text currently says `ft_irc`; align it with `ircserv` if polishing the interface.

## 2. Build and runtime constraints

- Compiler: `c++`
- Language flags: `-Wall -Wextra -Werror -std=c++98`
- Build: `make`; clean object files with `make clean`; remove all build products with `make fclean`; rebuild with `make re`.
- Use TCP/IPv4 sockets, bind to `INADDR_ANY`, listen, and monitor the listening socket and all connected client sockets with `poll`.
- IRC lines are terminated by `\r\n`. A complete IRC message, including its terminator, is limited to 512 bytes by the traditional protocol framing used here.
- A TCP `read` is not guaranteed to contain exactly one IRC line. Maintain a per-client input buffer, extract every complete line, and retain an incomplete suffix.
- Handle short reads, multiple commands in a read, disconnects, and partial writes robustly. Do not assume one `send` transmits the full reply.
- No third-party libraries are required.

## 3. Architecture at a glance

`main` validates arguments and starts `server`. The `server` owns the listening socket, poll set, connected clients, and channels. Each client has registration and input-buffer state plus references to joined channels. Each channel tracks membership, operators, topic, invitations, and modes. Parsed IRC lines become `message` values and are dispatched to command handlers. `RPL_MSG.hpp` centralizes some numeric reply templates; `tools` contains parsing and utility helpers.

Ownership should be explicit. A sensible design is for `server` to own clients and channels, while channel membership and client joined-channel lists hold non-owning references. Removing a client must remove it from every channel before destroying it. Remove empty channels when appropriate. Avoid duplicate raw-pointer ownership and stale references.

## 4. Classes and roles

The existing declarations are grouped in `components.hpp` under namespace `components`.

### `components::server`

The network and application coordinator. It should:

- Store the configured port and server password.
- Create/configure/bind/listen on the server socket.
- Maintain a `pollfd` collection and client lookup keyed by socket descriptor.
- Accept new clients and read input without blocking other clients.
- Frame input into IRC lines, parse lines, dispatch commands, and serialize/send replies.
- Own client and channel registries; look up clients by nickname and channels by name.
- Perform clean disconnect/QUIT handling, including channel cleanup and notifications.
- Gate commands by authentication and registration state.

Current public helpers include `initServer`, `getRequests`, `sendReply`, `disconnectClient`, `executeRequestedCommand`, `parseRequest`, `formulateReply`, channel registry helpers, and nickname lookup helpers. Command handler methods (`__PASS`, `__NICK`, etc.) are currently methods of `server`, implemented in `commands.cpp`.

### `components::client`

Represents one connected IRC user. It stores the socket descriptor, nickname, username, real name, hostname/server name, input buffer, authentication/registration flags, whether NICK/USER have been supplied, and the channels joined by the user. It provides identity/prefix formatting such as `:nick!user@host` and `:server`.

Registration should be complete only after a valid PASS (where required), NICK, and USER have been accepted. Do not mark a client registered merely because it sent malformed or duplicate registration commands. Keep per-client buffering independent so fragmented TCP input cannot mix between clients.

### `components::channel`

Represents an IRC channel. It stores the channel name and topic, member map, operator map, invite list, optional key/password, optional user limit, and mode state. It supports membership/operator/invite queries and channel mode formatting.

The intended channel modes in this implementation are:

- `i`: invite-only.
- `t`: only operators can change the topic.
- `k`: channel requires a key/password.
- `l`: channel has a user limit.
- `o`: grant/remove channel operator status for a member.
- `n`: prevent messages from non-members (the current `getChannelModes()` always includes `+n`).

Keep membership count consistent. Adding an already-present member must not increment it; removing a member must decrement it. Operator status is separate from membership, but operators are also channel members.

### `components::message`

Represents a parsed IRC command: optional prefix, command name, and ordered parameters. Sender/receiver pointers exist in the current declaration but parsing/dispatch primarily uses the parsed text and the `client` currently being processed. Prefer a simple value type with no unnecessary ownership or uninitialized pointer state.

### `tools`

Free helper functions in `tools.hpp` / `tools.cpp`. Current helpers cover logging, timestamp formatting, basic port/nickname validation, suffix checks, splitting, trimming, and password comparison. Parsing helpers should handle empty input safely, avoid out-of-range indexing, and preserve IRC trailing parameters correctly.

### Reply definitions (`RPL_MSG.hpp`)

Contains macros for welcome/channel replies and common numeric errors. These are templates only; the server must still produce correctly prefixed, space-delimited IRC lines ending in `\r\n`. Prefer functions or a dedicated formatter if macros make correctness difficult.

## 5. Source-file map

| File | Responsibility |
|---|---|
| `main.cpp` | Validate `<port> <password>`, construct and run the server, report startup exceptions. |
| `components.hpp` | Shared class declarations and required POSIX/C++ includes. |
| `server.cpp` | Socket setup, poll/read loop, line parsing, dispatch, client/channel registries, reply output, registration welcome. |
| `client.cpp` | Client state, identity accessors/prefix construction, joined-channel bookkeeping, input buffering. |
| `channel.cpp` | Channel membership, operators, invitations, topic/key/limit/mode state. |
| `commands.cpp` | IRC command handlers and command-specific validation/replies. |
| `message.cpp` | Parsed IRC message storage/accessors. |
| `tools.hpp`, `tools.cpp` | Utility declarations and implementations. |
| `RPL_MSG.hpp` | IRC numeric and command reply templates. |
| `Makefile` | C++98 compilation and linking of `ircserv`. |

## 6. IRC input and output model

An IRC command line has the general shape:

```text
[@optional-prefix ]COMMAND [middle-parameter ...] [:trailing parameter with spaces]\r\n
```

For client-originated commands, clients generally do not supply a trusted prefix; identify the sender from the socket's `client` object. Parse the command and parameters case-insensitively for command names, while preserving nickname/channel spelling where the server's chosen comparison rules require it. The colon introducing the trailing parameter is syntax and should be removed from the parameter value. A trailing parameter can be empty and can contain spaces.

Replies are also CRLF-terminated. Numerics conventionally use `:<server> <three-digit-code> <recipient> ...`; relayed user commands use the sender's prefix. Never concatenate a second server prefix onto a reply macro that already contains one.

Recommended read-loop procedure:

1. Call `poll` for the listening descriptor and connected descriptors.
2. Accept ready connections; set descriptors nonblocking and register them with `poll`.
3. For each readable client, read available bytes and append them to that client's buffer.
4. Repeatedly extract complete `\r\n`-terminated lines, validate the line length, parse each line, and dispatch it.
5. Retain only the incomplete final fragment. On EOF or fatal socket error, run common disconnect cleanup.
6. Handle poll events carefully if the descriptor vector changes during iteration; do not skip or access the wrong client after erasing an entry.

## 7. Registration and command flow

1. The client connects; create an unauthenticated, unregistered client record.
2. `PASS <server-password>` authenticates the connection. Missing or incorrect password must produce the appropriate error and close/deny access according to the project's policy.
3. `NICK <nickname>` validates and claims a unique nickname. Nickname changes must update channel membership indexes and notify relevant peers.
4. `USER <username> <hostname> <servername> :<real name>` supplies user identity. The client-provided host/server fields should not be trusted for security-sensitive decisions.
5. Once authentication and both NICK and USER are complete, send the welcome numerics (001–004) exactly once and allow registered-only commands.
6. Dispatch subsequent commands to handlers. Commands before registration should receive a registration error; commands requiring authentication should not be processed.

## 8. Commands to implement

The existing dispatcher recognizes the following commands. `CAP` is currently ignored to support clients that send capability negotiation on connect. At minimum, unknown commands should receive numeric 421.

| Command | Behavior and key checks |
|---|---|
| `PASS <password>` | Authenticate a connection; reject missing/wrong password and repeated PASS after registration/authentication as appropriate. |
| `NICK <nickname>` | Validate syntax/length, reject collisions, update indexes, broadcast nickname change to shared-channel peers. |
| `USER <user> <mode> <unused> :<real name>` | Store username and real name once; complete registration in combination with PASS/NICK. |
| `JOIN <channels> [keys]` | Join comma-separated channels, create absent channels, make creator operator, validate key/limit/invite-only modes, broadcast JOIN, send topic and names replies. `JOIN 0` may be supported as leave-all if desired by the project specification. |
| `PART <channels> [:reason]` | Leave one or more channels, notify members, remove empty channels, report missing/non-member cases. |
| `PRIVMSG <target>[,...] :<text>` | Send to a user or channel; do not echo to sender unless required; enforce channel membership/modes; report missing recipient/target and empty text appropriately. |
| `MODE <channel> [modes [arguments...]]` | Query modes or change `i`, `t`, `k`, `o`, `l`; require channel operator privileges for changes and validate required arguments. |
| `INVITE <nickname> <channel>` | Check target exists, channel exists, inviter permissions/membership, and whether target is already a member; record invitation and notify target/inviter. |
| `KICK <channel> <nickname>[,...] [:reason]` | Require operator privilege, verify target membership, notify channel, remove target (and operator status if needed). |
| `TOPIC <channel> [:topic]` | Query or set topic; require membership and, with `+t`, operator privilege; broadcast changes. |
| `WHO <mask>` | Report matching user or channel members and terminate with end-of-WHO numeric. |
| `QUIT [:reason]` | Notify shared channel members and disconnect cleanly. A missing reason should be allowed by common IRC behavior. |

The 42-style core channel-to-channel and user-to-user messaging behavior, registration, and operator mode checks are more important than implementing the full IRC RFC. Do not silently claim unsupported commands work.

## 9. Suggested implementation sequence

1. **Confirm the contract.** Read the project's assignment/specification if available. Settle supported modes, numeric replies, nickname/channel rules, and required command set.
2. **Define ownership and state.** Specify `Server`, `Client`, `Channel`, parsed-message, and output-buffer responsibilities before adding handlers. Initialize every field in constructors.
3. **Build reliable line framing and parser.** Make parsing independently understandable: one complete IRC line in, one command plus parameters out. Cover optional prefix and trailing parameter with spaces.
4. **Implement socket lifecycle.** Nonblocking listener/client sockets, `poll`, accept/read/write handling, and a single cleanup path for disconnects.
5. **Implement registration.** PASS/NICK/USER ordering, duplicate/collision checks, registration completion, welcome numerics, and command gating.
6. **Implement channels.** Join/part/membership cleanup, operators, topic, invitations, keys, limits, and empty-channel deletion.
7. **Implement messaging and commands.** PRIVMSG first, then MODE, INVITE, KICK, TOPIC, WHO, QUIT, and unknown-command errors.
8. **Review protocol formatting and error paths.** Check every reply's prefix, recipient, numeric, parameters, trailing colon, and CRLF. Verify state changes happen once and before notifications that depend on the new state.
9. **Build and exercise manually.** Use an IRC client or `nc`/`telnet` to connect multiple clients and verify fragmented/multiple lines, auth, registration, messages, channel operations, and disconnect cleanup.

## 10. Important current-code caveats

Treat the checked-in implementation as a starting point, not a complete protocol specification. Several visible defects should not be copied into a rebuild:

- `portIsValid` accepts an empty string and does not enforce a numeric range; `main` does not validate argument count safely before converting a valid port.
- The input loop constructs a string from a zero-filled fixed buffer instead of using the exact `read` byte count, and its complete-line handling drops/retains data incorrectly around the last newline.
- `parseRequest` has fragile colon and token handling, can index empty tokens, and does not robustly parse multiple spaces or trailing parameters.
- `server::listenFd`, several client fields, and channel limit state need complete initialization before use.
- A failed `accept` is logged but execution still proceeds as if the descriptor were valid.
- Poll-vector mutation while iterating can invalidate the current index. Disconnect handling should remove the client from channels and avoid using a destroyed client.
- `sendReply` assumes all bytes are sent at once. Nonblocking sockets need an output queue or a correct retry strategy.
- Client buffering currently clears the entire buffer on overflow and uses an incorrect 512-byte accumulation approach.
- Channel membership/operator bookkeeping has inconsistencies: operator membership is stored separately, `removeClient` increments the member count, duplicate insertion is not handled, and empty channels are not consistently removed.
- Some handlers use the first requested channel rather than the loop's current channel; some command arguments and reasons are not formatted or validated consistently.
- `QUIT` currently requires a reason, and teardown paths are not unified.
- Numeric reply macros and `formulateReply` can produce malformed lines if their prefixes/parameters are combined inconsistently.
- `CAP` is intentionally ignored; modern IRC clients may expect at least a harmless `CAP LS` response depending on the test client.

Fix correctness and memory/lifecycle problems before extending command coverage. Keep behavior aligned with the assignment requirements rather than trying to implement every feature of the full IRC standard.

## 11. Completion checklist

- The project builds cleanly with the required compiler flags.
- The server validates `<port> <password>` and reports startup failures clearly.
- Several clients can connect and remain responsive concurrently.
- Registration succeeds only after correct password, nickname, and USER data; welcome is sent once.
- Input fragmentation, batched commands, CRLF boundaries, and the 512-byte limit are handled without dropping adjacent commands.
- Nicknames are unique; channel names, members, operators, invitations, topics, keys, and limits remain consistent.
- Required commands generate correct state changes and CRLF-terminated replies.
- EOF, QUIT, send/read errors, and server shutdown release descriptors and references safely.

