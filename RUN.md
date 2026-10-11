# Running ft_irc

## Build

```bash
cd ft_irc
make
```

Rebuild from scratch:

```bash
make re
```

## Start the server

```bash
./ft_irc <port> <password>
# example:
./ft_irc 6667 secret
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

## Use the python test client

```bash
python3 test_client.py
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
