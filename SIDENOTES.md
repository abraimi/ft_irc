# Things from the rfc:
### 1. Clients [RFC 1459-1.2](https://www.rfc-editor.org/rfc/rfc1459.html#section-1.2):
- Nicknames should be unique and a max of 9 chars.
- The server must have the following information about the client:
	- Real name of the host.
	- Username of the client.
	- And the server in which he's connected.

### 2. Operators [RFC 1459-1.2.1](https://www.rfc-editor.org/rfc/rfc1459.html#section-1.2.1):


### 3. Channels [RFC 1459-1.3](https://www.rfc-editor.org/rfc/rfc1459.html#section-1.3):
- Channels names are strings (beginning with a '&' or '#' character) of
   length up to 200 characters.
- Channel name must not contain the following characters(' ', ASCII(7), ',').
- The channel created with a leading '&' is and must be discoverable by all the servers in the network.
- Upon joining a channel if it doesn't exist the channel is created and the client becomes the channel operator.