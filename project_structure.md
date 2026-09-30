```
ft_irc/
│
├── Makefile
├── README.md
│
├── include/
│   │
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   ├── Command.hpp              # Shared interface for parsed commands
│   │
│   └── commands/
│       ├── PassCommand.hpp
│       ├── NickCommand.hpp
│       ├── UserCommand.hpp
│       ├── JoinCommand.hpp
│       ├── PartCommand.hpp
│       ├── PrivmsgCommand.hpp
│       ├── QuitCommand.hpp
│       ├── KickCommand.hpp
│       ├── InviteCommand.hpp
│       ├── TopicCommand.hpp
│       └── ModeCommand.hpp
│
├── src/
│   │
│   ├── main.cpp
│   │
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── Channel.cpp
│   ├── Command.cpp
│   │
│   └── commands/
│       ├── PassCommand.cpp
│       ├── NickCommand.cpp
│       ├── UserCommand.cpp
│       ├── JoinCommand.cpp
│       ├── PartCommand.cpp
│       ├── PrivmsgCommand.cpp
│       ├── QuitCommand.cpp
│       ├── KickCommand.cpp
│       ├── InviteCommand.cpp
│       ├── TopicCommand.cpp
│       └── ModeCommand.cpp


```

# retahri Responsibility

## 1 / Server

Server.hpp
Server.cpp

handling the following :
socket()
bind()
listen()
accept()
fcntl()
poll()
recv()
send()
close()

the server event loop :

```
             poll()
                │
       ┌────────┼────────┐
       ▼        ▼        ▼
    listen     Client   Client
     ready     ready    ready
       │        │        │
    accept()   recv()   recv()
```

## 2 / Client

Client.hpp
Client.cpp

maintain the state of each connected client:

```
Client
│
├── fd
├── nickname
├── username
├── hostname
├── authenticated
├── receive buffer
├── send buffer
└── channels
```

server needs this information regardless of how parsing is implemented :

```
fd = 7
nickname = "reda"
username = "reda"
authenticated = true
```

## 3 / Channel

channel.hpp
channel.cpp

This is especially important because the mandatory features depend heavily on IRC channel state.

```
Channel #42
│
├── members
├── operators
├── topic
├── password
├── invite list
├── invite-only mode
├── topic mode
└── user limit
```

after parsing commands i will have to : (for examle command JOIN)

```
Parser
   │
   │ "JOIN #42"
   ▼
┌─────────────┐
│ JOIN parsed │
└──────┬──────┘
       │
       ▼
    Server
       │
       ▼
 Channel #42
       │
       ▼
 add client
       │
       ▼
 broadcast JOIN
```

shadow determines WHAT the client said.

I determine WHAT THE SERVER DOES.

# abraimi responsibility

the simple class representation of parsing

```cpp
class Command
{
private:
    std::string _name;
    std::vector<std::string> _arguments;

public:
    const std::string &getName() const;
    const std::vector<std::string> &getArguments() const;
};
```

we can agree on smthg like :

```
Command
│
├── name = "PRIVMSG"
│
└── arguments
      ├── "#42"
      └── "hello world"
```

! ps :

The subject explicitly requires the server to correctly aggregate TCP data before processing commands.

So even if you (shadow) owns the **IRC parser**, we need to agree on **who owns the raw TCP receive buffer and `\r\n` message extraction**.

i recommend :

```
Server/Client
    │
    │ receives TCP chunks
    ▼
Client receive buffer
    │
    │ extract complete IRC lines
    ▼
Parser
    │
    │ parse IRC syntax
    ▼
Command
```

i will give you one IRC command
you give me smthg like : This line means `PRIVMSG`, target is `#42`, message is `hello`.

### AN ADDITIONAL FILE WILL BE ON THE ROOT OF THIS REPO ```UPDATES``` TO COMMUNICATE PROBLEMS AND NOTES SINCE KATCHUBAR LIL WANA SBA7 👯‍♂
