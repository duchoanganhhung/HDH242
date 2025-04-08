#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

/* Check queue is empty */
int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return q->size == 0;
}

/* Put new process to queue */
void enqueue(struct queue_t * q, struct pcb_t * proc) {
        if (q->size == MAX_QUEUE_SIZE || q == NULL) return;
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->proc[q->rear] = proc;
        q->size++;
}

/* Get process from queue */
struct pcb_t * dequeue(struct queue_t * q) {
        if (empty(q)) return NULL;
        struct pcb_t * proc = q->proc[q->front];
        q->front = (q->front + 1) % MAX_QUEUE_SIZE;
        q->size--;
        if (q->size == 0) {
                q->front = 0;
                q->rear = -1;
        }
	return proc;
}

/* Reset */
void reset_queue_if_zero(struct queue_t * q){
        if (q == NULL) return;
        if (q->size == 0){
                q->front = 0;
                q->rear = -1;
        }
}

/* Compress queue, that is, if the queue has some space between element, those space will be remove */
void compress_queue(struct queue_t * q) {
        if (q == NULL) return;
        int left_pointer = q->front;
        int right_pointer = q->front;

        int final_size = 0;
        int i = 0;

        while (i < q->size){
                if (q->proc[right_pointer] != NULL) {
                        q->proc[left_pointer] = q->proc[right_pointer];
                        left_pointer = (left_pointer + 1) % MAX_QUEUE_SIZE;
                        final_size++;
                }
                right_pointer = (right_pointer + 1) % MAX_QUEUE_SIZE;
                i++;
        }

        q->size = final_size;
        
        if (q->size == 0) {
                q->front = 0;
                q->rear = -1;
        } else {
                q->rear = (q->front + final_size - 1) % MAX_QUEUE_SIZE;
        }
}