Our goal is to create an IRC ( internet relay chat ) server on port 6667 and require a password when clients authenticate.

./irc 6667 password

the sequence will be

socket() -> setsockopt() -> bind() -> listen() -> accept()

socket() = create a socket AKA give me a network communication endpoint , might returns someting like fd = 3. why ?? because a socket is a file descriptor and that fd will be the endpoint.

bind() = will attach this socket to this address/port : socket -> bind -> 127.0.0.1:6667.

listen() = tells the os that this socket is a server socket, wait for incoming connections :

Internet
│
▼
┌─────────┐
│ ircserv │
│ :6667   │
└─────────┘
▲
│
waiting...

accept() = accepts that connection and gives you another fd for example :

listenting socket -> accept() -> client socket

fd 3 will be listening socket while fd 4 -> reda, fd 5 -> shadow, fd 6 -> bob

in nutshell the listening socket is used to accept new clients while client sockets are used to communicate with clients fd 3.

PS: for multiple clients we cant simply do :

accept();

recv(reda);

recv(shadow);

recv(bob);

because reda might stop sending data and the server will get stuck waiting for more data from data meanwhile shadow and bob's data is hanged waiting for servel to recv.

!!! man recv

poll() = works as a security guard watching those senders

instead of asking is reda sending something ? is shadow sending something ? etc...

poll asks the os wich file descriptor is ready if reda and shadow sockets are ready and bob isnt my server will only handle the ready ones.

why it non blocking ? because it doesnt make the whole program wait for one socket

```
poll()
                          │
┌──────────┼──────────┐
▼                      ▼                        ▼
server              Reda                  Alice
ready              ready                    no
│                      │
▼                    ▼
accept()          recv()
```

this will allow one process to handke many clients without the need of fork() or another process/threads

# event loop

what the server should look like

```
while (server_is_running)
{
    poll(...);

    if (server_socket is ready)
        accept new client;

    if (client_socket is ready)
        receive data;

    process commands;

    send responses;
}
```

!!!PS : tcp doesnt preserve commands it a stream, and as stated in the subject the user might send "co" "mma" "nd" "\n" instead of "command '\'\n" and we must reconstruct it before processing to fix this every client needs a receive buffer packet 1 = com next packet  = and\n there that we can process it as follows "command\n "


in the subject we have ```nc``` is smthg wtf !! it let you bypass hexChat and manually speack IRC it basically tells the os , let me pretend im an IRC client using the terminal. ```man nc```

# the big picture


SERVER   PORT 6667 --> poll() ---> multiple clients ---> each clients has a buffer ---> IRC parser ---> IRC command = if PASS -> client state , if JOIN -> Channel,

if PRIVMSG -> Broadcast
