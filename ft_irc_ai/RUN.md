# Running ft_irc

## Build

```bash
cd ft_irc_ai
make
```

Rebuild from scratch:

```bash
make re
```

## Start the server

```bash
./ft_irc_ai <port> <password>
# example:
./ft_irc_ai 6667 secret
```

## Connect with netcat

```bash
nc 127.0.0.1 6667
```

Then type IRC commands:

```
PASS secret
NICK reda
USER reda 0 * :Reda
JOIN #42
PRIVMSG #42 :hello
TOPIC #42 :my topic
MODE #42 +i
INVITE bob #42
KICK #42 bob :bye
PART #42 :goodnight
QUIT :bye
```

## Connect with a real IRC client (HexChat)

- Network/server: `127.0.0.1/6667`
- Password: `secret`
- Set your nickname, then `/join #42` and chat.

## Use your python test client

```bash
python3 ../test_client.py
```

(Edit `PORT` in `test_client.py` if you chose a different port.)

## Stop the server

```bash
Ctrl+C
```

## Clean build files

```bash
make clean   # remove build objects
make fclean  # remove objects + binary
```
