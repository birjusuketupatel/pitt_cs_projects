# BROADCAST PROTOCOL CONSTANTS

RECV_BUFFER_SIZE = 1024
DEBUG_BROADCAST = False         # print debug messages corresponding to the broadcast protocol
LOSS = False                    # call sendto_dbg instead of sendto
PROB = 10                       # sendto_dbg has 1/PROB chance of sending a packet
ACK_TIME = 10                   # after receiving ACK_TIME messages, broadcast an ack
NACK_TIMEOUT = 3                # node broadcasts nacks every NACK_TIMEOUT seconds
VIEW_TIMEOUT = 10               # if we have not heard from a server in 10 seconds, it is no longer in our view
N_SERVERS = 5                   # number of replicas in the system

# SERVER PROTOCOL CONSTANTS

DEBUG_SERVER = False            # prints server state after each modification
PORT_NUM = 18861                # port number where all clients can connect to