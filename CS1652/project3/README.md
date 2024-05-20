# CS1652-project3

University of Pittsburgh
CS1652 Project 2
Due: April 24, 2022

Members:
  - Birju Patel
  - Carter Moriarity

Contributions:
  - Birju implemented the link state routing algorithm.
  - Birju implemented the use of heartbeats to monitor if an edge was alive.
  - Carter implemented the distance vector routing algorithm.
  - Birju added the poisoned reverse technique to Carter's implementation.

Notes:
  - When debug mode is on, all given debug messages will be printed. Also, each
  time there is a change to the forwarding table, the new forwarding table,
  new minimum paths, and distance vector table (if applicable) will be printed.
  - Our algorithm waits for 10 missed heartbeats before declaring an edge dead.
  - A heartbeat is sent once per second.

Our Implementation:
  - For our link state packet forwarding protocol, we forwarded an LSA
  if the LSA caused a change in state of our forwarding table. Otherwise,
  the LSA was dropped.
  - This is not a reliable protocol. As Birju mentioned in the Slack, it will
  lead to some nodes not knowing about certain edges under specific circumstances.
  - We used a similar protocol for distance vector broadcasting. We broadcast
  to neighbors if the vector caused a change in our distance vector.
  - When implementing distance vector, Carter ran into the count to infinity
  problem. Birju fixed this by using the poisoned reverse technique, as
  discussed in the textbook.
