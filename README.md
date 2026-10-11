*This project has been created as part of the 42 curriculum by abraimi, goldinooo.*

# Description

`ircserv` is a small IRC server written in C++98. It accepts plain TCP connections,
authenticates clients with a configured password, registers nicknames and usernames,
and supports channel and private messaging. Channel operators can manage membership,
topics, invitations, and the required channel modes. One `poll()` loop handles the
listening socket and all clients; client sockets are non-blocking.

# Instructions

Build with the required compiler warnings:

```sh
make
```

Run the server with a port and connection password:

```sh
./ircserv 6667 mypassword
```

Connect manually using netcat (the `-C` option sends IRC CRLF line endings):

```sh
nc -C 127.0.0.1 6667
```

Then enter:

```text
PASS mypassword
NICK reda
USER reda 0 * :Reda Tahri
JOIN #test
PRIVMSG #test :hello everyone
```

Use `make clean` to remove objects, `make fclean` to remove objects and the executable,
and `make re` to rebuild from scratch. The parser checks can be run with `make test`.

## HexChat

Add a network/server for `127.0.0.1/6667`, disable TLS/SSL, and set the server
password to the value passed to `ircserv`. Choose a nickname and connect. This server
does not provide TLS.

# Features

- PASS, NICK, USER registration and IRC welcome numerics.
- JOIN, channel and private PRIVMSG, channel membership and initial channel operator.
- KICK, INVITE, TOPIC, and channel modes `i`, `t`, `k`, `o`, and `l`.
- CRLF framing with per-client input buffers, multiple commands per read, and buffered
  output for partial writes.
- Minimal CAP and PING handling for common IRC-client connection behavior.

# Resources

- [42 ft_irc subject](en.subject%20(1).pdf)
- [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `poll(2)`, `socket(2)`, `recv(2)`, and `send(2)` manual pages.

AI assistance was used to review the integration boundary, identify defects, help
implement and refine parser/server integration, and design protocol test cases. The
resulting code and behavior must be reviewed and understood by the project authors.
