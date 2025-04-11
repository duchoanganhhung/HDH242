#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if(q->size < MAX_QUEUE_SIZE){
        	q->proc[q->size] = proc;
		#ifdef MLQ_SCHED
		if(proc->priority != proc->prio){
			proc->priority = proc->prio;
		}
		#endif
		q->size ++;
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if(q->size > 0){
		unsigned long Max_priority = q->proc[0]->priority;
		unsigned long position = 0;
		for(unsigned long i = 1; i < q->size; i++){
			if(q->proc[i]->priority < Max_priority){
				Max_priority = q->proc[i]->priority;
				position = i;
			}
		}
		struct pcb_t * res = q->proc[position];
		for(unsigned long i = position; i < q->size; i++){
			q->proc[i] = q->proc[i+1];
		}
		q->size --;
		return res;
	}else{
		return NULL;
	}

}



