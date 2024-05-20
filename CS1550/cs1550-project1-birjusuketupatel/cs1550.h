#ifndef _LINUX_CS1550_H
#define _LINUX_CS1550_H

#include <linux/list.h>

/**
 * A generic semaphore, providing serialized signaling and
 * waiting based upon a user-supplied integer value.
 */
struct cs1550_sem
{
	/* Current value. If nonpositive, wait() will block */
	long value;

	/* Sequential numeric ID of the semaphore */
	long sem_id;

	/* Per-semaphore lock, serializes access to value */
	spinlock_t lock;

	/* FIFO queue of tasks waiting on this resource */
	struct list_head waiting_tasks;

	/* link to next and previous semaphore in global semaphore list */
	struct list_head list;
};

/**
	* A wrapper for putting task data in a list.
*/
struct cs1550_task{
	struct task_struct *task;
	struct list_head list;
};

#endif
