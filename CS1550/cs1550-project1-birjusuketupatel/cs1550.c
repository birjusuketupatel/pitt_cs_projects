#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/cs1550.h>

/* global semaphore list */
static LIST_HEAD(sem_list);
static DEFINE_RWLOCK(sem_rwlock);

/* next semaphore id */
unsigned long next_id = 0;

/**
 * Creates a new semaphore. The long integer value is used to
 * initialize the semaphore's value.
 *
 * The initial `value` must be greater than or equal to zero.
 *
 * On success, returns the identifier of the created
 * semaphore, which can be used with up() and down().
 *
 * On failure, returns -EINVAL or -ENOMEM, depending on the
 * failure condition.
 */
SYSCALL_DEFINE1(cs1550_create, long, value)
{
	//value must be >= 0
	if(value < 0){
		return -EINVAL;
	}

	struct cs1550_sem *next_sem = (struct cs1550_sem *) kmalloc(sizeof(struct cs1550_sem), GFP_KERNEL);

	//allocation failed, not enough memory
	if(next_sem == NULL){
		return -ENOMEM;
	}

	//initialize data contained within the semaphore
	next_sem->value = value;
	unsigned long id = next_id;
	next_sem->sem_id = id;
	next_id++;
	spin_lock_init(&next_sem->lock);
	INIT_LIST_HEAD(&next_sem->waiting_tasks);

	//add to global semaphore list
	read_lock(&sem_rwlock);
	INIT_LIST_HEAD(&next_sem->list);
	list_add(&next_sem->list, &sem_list);
	read_unlock(&sem_rwlock);

	return id;
}

/**
 * Performs the down() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This decrements the value of the semaphore, and *may cause* the
 * calling process to sleep (if the semaphore's value goes below 0)
 * until up() is called on the semaphore by another process.
 *
 * Returns 0 when successful, or -EINVAL or -ENOMEM if an error
 * occurred.
 */
SYSCALL_DEFINE1(cs1550_down, long, sem_id)
{
	read_lock(&sem_rwlock);
	//search sem_list for the semaphore
	struct cs1550_sem *sem = NULL;
	bool found = false;
	list_for_each_entry(sem, &sem_list, list){
		//sem_id found
		if(sem->sem_id == sem_id){
			found = true;
			break;
		}
	}

	//semaphore not in list
	if(!found){
		read_unlock(&sem_rwlock);
		return -EINVAL;
	}
	read_unlock(&sem_rwlock);

	//consume the resource
	spin_lock(&sem->lock);
	sem->value--;

	//put task to sleep
	if(sem->value < 0){
		struct cs1550_task *to_sleep = (struct cs1550_task *) kmalloc(sizeof(struct cs1550_task), GFP_ATOMIC);

		//not enough memory
		if(to_sleep == NULL){
			spin_unlock(&sem->lock);
			return -ENOMEM;
		}

		//put current task to sleep and pick next task
		to_sleep->task = current;
		INIT_LIST_HEAD(&to_sleep->list);
		list_add_tail(&to_sleep->list, &sem->waiting_tasks);

		set_current_state(TASK_INTERRUPTIBLE);
		spin_unlock(&sem->lock);
		schedule();
	}
	else{
		spin_unlock(&sem->lock);
	}

	return 0;
}

/**
 * Performs the up() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This increments the value of the semaphore, and *may cause* the
 * calling process to wake up a process waiting on the semaphore,
 * if such a process exists in the queue.
 *
 * Returns 0 when successful, or -EINVAL if the semaphore ID is
 * invalid.
 */
SYSCALL_DEFINE1(cs1550_up, long, sem_id)
{
	read_lock(&sem_rwlock);
	//search sem_list for the semaphore
	struct cs1550_sem *sem = NULL;
	bool found = false;
	list_for_each_entry(sem, &sem_list, list){
		//sem_id found
		if(sem->sem_id == sem_id){
			found = true;
			break;
		}
	}

	//semaphore not in list
	if(!found){
		read_unlock(&sem_rwlock);
		return -EINVAL;
	}
	read_unlock(&sem_rwlock);

	//release the resource
	spin_lock(&sem->lock);
	sem->value++;

	//wake up next process in queue
	if(sem->value <= 0 && !list_empty(&sem->waiting_tasks)){
		struct cs1550_task *to_wake = list_first_entry(&sem->waiting_tasks, struct cs1550_task, list);
		list_del(&to_wake->list);
		wake_up_process(to_wake->task);
		kfree(to_wake);
	}
	spin_unlock(&sem->lock);

	return 0;
}

/**
 * Removes an already-created semaphore from the system-wide
 * semaphore list using the identifier obtained from a previous
 * call to cs1550_create().
 *
 * Returns 0 when successful or -EINVAL if the semaphore ID is
 * invalid or the semaphore's process queue is not empty.
 */
SYSCALL_DEFINE1(cs1550_close, long, sem_id)
{
	read_lock(&sem_rwlock);
	//search sem_list for the semaphore
	struct cs1550_sem *sem = NULL;
	bool found = false;
	list_for_each_entry(sem, &sem_list, list){
		//sem_id found
		if(sem->sem_id == sem_id){
			found = true;
			break;
		}
	}

	//semaphore not in list or has elements in its process queue
	if(!found || !list_empty(&sem->waiting_tasks)){
		read_unlock(&sem_rwlock);
		return -EINVAL;
	}

	//remove and free semaphore
	list_del(&sem->list);

	kfree(sem);
	read_unlock(&sem_rwlock);

	return 0;
}
