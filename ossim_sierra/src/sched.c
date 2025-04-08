
#include "queue.h"
#include "sched.h"
#include "libmem.h"
#include "loader.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif

int queue_empty(void) {
	#ifdef MLQ_SCHED
		unsigned long prio;
		for (prio = 0; prio < MAX_PRIO; prio++)
			if(!empty(&mlq_ready_queue[prio])) 
				return -1;
	#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
	#ifdef MLQ_SCHED
		for (int i = 0; i < MAX_PRIO; i ++) {
			mlq_ready_queue[i].size = 0;
			mlq_ready_queue[i].front = 0;
			mlq_ready_queue[i].rear = -1;
			slot[i] = MAX_PRIO-i;
		}

		running_list.size = 0;
		running_list.front = 0;
		running_list.rear = -1;
	#endif
		ready_queue.size = 0;
		run_queue.size = 0;
		pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
// Get a process from mlq ready queue, put it to running list
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;
	int is_slot_full = 1;
	int existed_process_queue_idx = -1;
	int process_existed;

	pthread_mutex_lock(&queue_lock);
	for (int i = 0; i < MAX_PRIO; i++) {
		process_existed = 0;
		if (!empty(&mlq_ready_queue[i])) {
			process_existed = 1;
			existed_process_queue_idx = i;
		}
		if (slot[i] > 0){
			is_slot_full = 0;
			if (process_existed) {
				slot[i]--;
				proc = dequeue(&mlq_ready_queue[i]);
				enqueue(&running_list, proc);
				pthread_mutex_unlock(&queue_lock);
				return proc;
			}
		}
	}

	if (is_slot_full){
		for (int i = 0; i < MAX_PRIO; i++) {
			slot[i] = MAX_PRIO - i;
		}
	}

	if (existed_process_queue_idx != -1){
		proc = dequeue(&mlq_ready_queue[existed_process_queue_idx]);
		enqueue(&running_list, proc);
		pthread_mutex_unlock(&queue_lock);
		return proc;
	}

	pthread_mutex_unlock(&queue_lock);
	return NULL;
}

/* Remove a process from running_list */
void get_out_running_list(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	if (empty(&running_list)) {
		pthread_mutex_unlock(&queue_lock);
		return;
	}
	int i = running_list.front;
	while (1) {
		if (running_list.proc[i] == proc) {
			running_list.proc[i] = NULL;
			int j = i;
			while (j != running_list.rear) {
				running_list.proc[j] = running_list.proc[(j + 1) % MAX_QUEUE_SIZE];
				j = (j + 1) % MAX_QUEUE_SIZE;
			}
			running_list.size--;
			running_list.rear = (running_list.rear - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
			reset_queue_if_zero(&running_list);
			break;
		}
		i = (i + 1) % MAX_QUEUE_SIZE;
		if (i == (running_list.rear + 1) % MAX_QUEUE_SIZE) break;
	}
	pthread_mutex_unlock(&queue_lock);
}

// Put a process back to mlq ready queue from running list
void put_mlq_proc(struct pcb_t * proc) {
	get_out_running_list(proc);
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

// Don't care
void add_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

// Don't care
struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

// Don't care
void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = & running_list;

	return put_mlq_proc(proc);
}

// Don't care
void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = & running_list;

	return add_mlq_proc(proc);
}

void kill_process(const char * proc_name){
	char full_proc_name[100] = {}; 
	strcat(full_proc_name, "input/proc/");
	strcat(full_proc_name, proc_name);
	struct pcb_t * proc;
	pthread_mutex_lock(&queue_lock);
	for (int i = 0; i < MAX_PRIO; i++) {
        struct queue_t *queue = &mlq_ready_queue[i];

		if (empty(queue)) continue;

        int j = queue->front;
        while (1) {
            proc = queue->proc[j];
			printf("%s\n%s\n", proc->path, full_proc_name);
			if (proc->pid == -1) {
				j = (j + 1) % MAX_QUEUE_SIZE;
				if ((j == queue->rear + 1) % MAX_QUEUE_SIZE) break;
				continue;
			}
            if (strcmp(proc->path, full_proc_name) == 0) {
                printf("Terminating process with PID: %d\n", proc->pid);
                queue->proc[j] = NULL;
                free_pcb_memph(proc);
				//process_cleanup(proc);
                free(proc);
            }
            j = (j + 1) % MAX_QUEUE_SIZE;
            if ((j == queue->rear + 1) % MAX_QUEUE_SIZE) break;
        }
        compress_queue(queue);
    }

	if (!empty(&running_list)){
		int j = running_list.front;
		while (1){
			proc = running_list.proc[j];
			if (strcmp(proc->path, full_proc_name) == 0) {
				printf("Terminating process with PID: %d\n", proc->pid);
				running_list.proc[j]->pid = -1;
			}
			j = (j + 1) % MAX_QUEUE_SIZE;
			if ((j == running_list.rear + 1) % MAX_QUEUE_SIZE) break;
		}
	}

	pthread_mutex_unlock(&queue_lock);
}

#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif