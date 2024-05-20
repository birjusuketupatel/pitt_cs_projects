# CS2510Project2

## Note

We have used the test script, ` test_p2.py ` which was given to us. The only modification we made to the script was changing the assumed function to run a server from `./chat_server` to `python3 chat_server.py -file cluster_config` in accordance with how our program runs. We also could not figure out a way to compile our python code to an executable such that it could be run in the form provided.

## makefile targets

Create docker image & containers for all five servers (through provided test file):
` make run `
  - Servers automatically start on container/bridge network creation

Reset docker containers (through provided test file):
` make clean `

Open bash shell for client process to run:
` docker run -it --cap-add=NET_ADMIN --network cs2510 --rm --name <unique container name> cs2510_p2 /bin/bash `

## Once in a client container shell

Establish a partition:
` python3 test_p2.py partition <partitions> `

Kill/Relaunch a server:
` python3 test_p2.py kill <server id> `
` python3 test_p2.py relaunch <server id> `

Establish loss between two servers:
` python3 test_p2.py loss <server id 1> <server id 2> <loss rate> `

Start client program:
` python3 client.py `

## Once inside client program

Help (see list of commands):
` ? `

Connect to running chat server on server hostname (depends on server id), port 18861:
- ID 1: ` c 172.30.100.101:18861 `
- ID 2: ` c 172.30.100.102:18861 `
- ID 3: ` c 172.30.100.103:18861 `
- ID 4: ` c 172.30.100.104:18861 `
- ID 5: ` c 172.30.100.105:18861 `

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

Print this server's view of other servers it can communicate with:
` v `

Quit client program:
` q `
