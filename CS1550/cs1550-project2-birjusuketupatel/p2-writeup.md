# Birju Patel, Project 2 Writeup
### Pseudocode for each object
* Visitor
  * arrive
  * consume ticket if available, leave if unavailable
  * wait outside museum for guide's signal, broadcast to guides that they are waiting
  * on signal from guide, tour museum
  * exit museum, broadcast to guides they have left
  * end execution
* Guide
  * arrive
  * if museum has 2 guides already, wait outside
  * enter museum
  * serve 10 visitors
      * if no more visitors are coming, exit loop
      * while no visitors are waiting, wait
      * if visitors are waiting, admit 1 visitor
  * while visitors are still in the museum, wait
  * when final guide leaves, signal next guide can enter

### Deadlock Prevention
Guides sleep if no visitors are waiting and wakes up as soon as a visitor enters.
Visitor signals to guide that it is waiting after it acquires a ticket.
If no more visitors will arrive, indicated by tickets=0, guides will not sleep
and immediately exit the loop. This prevents the condition of a guide sleeping
on a visitor which will never arrive.

### Starvation Prevention
Threads sleep when waiting, busy waiting is never used.
Visitors sleep when museum is at max capacity and when they are waiting for entry.
Guides sleep when there are 2 guides already inside, when no visitors are waiting,
and when they have completed their admitting phase and there are still visitors in the museum.
No thread holds the lock when it is idle.
