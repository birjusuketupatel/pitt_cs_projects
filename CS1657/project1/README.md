Implements a simple client and server program.\
The server is vulnerable to Paul Kocher's timing attack, as shown in the report.\

To compile, navigate to app directory and run javac -cp .:* *.java.\
To run the server, run java Server [port].\
To run the client, run java Client [serverIP]:[serverPort].
