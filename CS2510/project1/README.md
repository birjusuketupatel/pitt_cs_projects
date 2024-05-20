# CS2510Project1

Create docker image & container and open bash:
` make `

Reset docker image & container:
` make clear `

Open bash shell on currently existing docker container:
` make bash `

### Once inside docker container shell

Start a chat server on localhost, port 18861:
` python3 server.py `

Start a chat client instance:
` python3 client.py `

### Once inside client program

Help (see list of commands):
` ? `

Connect to running chat server on localhost, port 18861:
` c localhost:18861 `

Set username (once connected):
` u <username> `

Join group (once username is set):
` j <group name> `

Open prompt for new message (write new message after running command):
` a `

Like a message:
` l <message id> `

Remove your like on a message:
` r <message id> `

Print full group message log:
` p `

Quit client program:
` q `
