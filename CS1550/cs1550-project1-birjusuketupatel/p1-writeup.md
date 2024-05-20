# Birju Patel, Project 1 Writeup
### Using a FIFO queue to implement the global semaphore list
* PROS
	* I used the pre-built linked list API provided by Linux.
	* It was very easy to implement.
* CONS
	* To find a single semaphore, the computer must search through the entire global semaphore list.
	* In the worst case, it searches through the entire list, giving the FIFO queue an O(n) lookup time.
### Using a hash table
* PROS
	* You can find a particular semaphore quickly if we are given a key (i.e. semaphore id).
	* An O(1) lookup time to find a particular semaphore.
* CONS
	* It is harder to implement this, as I would have to do it from scratch.
