*This project has been created as part of the 42 curriculum by abraimi, retahri.*

# ft_irc

An IRC server implemented in C++98.

# Description

`ircserv` is a learning project that implements the server side of the Internet Relay
Chat (IRC) protocol. IRC clients connect over TCP, register with a server, exchange
private messages, and communicate in shared channels. This server handles multiple
clients without forking, using non-blocking sockets and a single `poll()` event loop.

The project implements the required client registration, channel messaging, and channel
operator features. It is an IRC server, not an IRC client, and does not implement
server-to-server communication or TLS.

# Features

- Password authentication and client registration with `PASS`, `NICK`, and `USER`.
- Channel membership and messaging with `JOIN` and `PRIVMSG`; channel messages are
  forwarded to the other channel members, and private messages can be sent to users.
- Channel operator and regular-user roles.
- Operator commands: `KICK`, `INVITE`, `TOPIC`, and `MODE` flags `i`, `t`, `k`, `o`,
  and `l`.
- Buffered IRC line handling for partial TCP reads, and buffered output for partial
  writes.
- Basic `CAP` and `PING` handling for common IRC-client connection behavior.

# Instructions

## Requirements

- `make`
- A C++ compiler (`c++`) that supports C++98
- An IRC client for interactive use; WeeChat is used as the reference client

No external libraries are required.

## Build

From the repository root, compile the server with the Makefile. It uses C++98 and these
compiler flags:

```text
-Wall -Wextra -Werror -std=c++98
```

```sh
make
```

## Run

Start the server with a listening port and connection password:

```sh
./ircserv <port> <password>
```

For example:

```sh
./ircserv 6667 mypassword
```

The port must be between 1 and 65535, and the password must not be empty.

## Connect with an IRC client

In WeeChat, add a server at `127.0.0.1` on port `6667`, disable TLS/SSL, and set the
server password to the same value passed to `ircserv`. Choose a nickname and connect.
This server does not provide TLS.

You can also connect manually with netcat. The `-C` option sends IRC CRLF line endings:

```sh
nc -C 127.0.0.1 6667
```

After connecting, enter IRC commands one per line:

```text
PASS mypassword
NICK Homer
USER Homer 0 * :Homer Simpson
JOIN #test
PRIVMSG #test :hello everyone
```

To try private messaging, connect a second client, register it with another nickname,
and send `PRIVMSG <nickname> :a private message` from either client.

Stop the server with `Ctrl+C`. The Makefile also provides these cleanup targets:

- `make clean` removes build objects.
- `make fclean` removes build objects and the `ircserv` executable.
- `make re` rebuilds from scratch.

# Resources

- [42 ft_irc subject](en.subject.pdf) — project requirements and evaluation guidance.
- [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459) — the original IRC protocol
  specification.
- [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812) — IRC client protocol
  specification.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) — practical
  introduction to sockets and network programming.
- Unix manual pages: `poll(2)`, `socket(2)`, `bind(2)`, `recv(2)`, and `send(2)`.

Ai was used in this project to understand the core concepts and RFC commands
and error handling in the server, it was also used to simplify the definition 
of the unix definitions and descriptions of the function calls used to 
implement the codebase.
