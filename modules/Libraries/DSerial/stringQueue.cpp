#include "stringQueue.h"

int stringQueueInit(stringQueue_t *q, uint8_t size) {
	void *ptr = malloc(size * sizeof(char*));
	if(ptr) {
		q->data = (char **)ptr;
		q->size = size+1;
		q->head = 0;
		q->tail = 0;
		return 1;
	}
	return 0;
}

int stringQueueAdd(stringQueue_t *q, char* s) {
	if(!stringQueueIsFull(q)) {
		q->data[q->head] = s;
		q->head = (q->head + 1) % q->size;
		return 1;
	}
	return 0;
}

char *stringQueueRemove(stringQueue_t *q) {
	char *retval;
	if(!stringQueueIsEmpty(q)) {
		retval = q->data[q->tail];
		q->tail = (q->tail + 1) % q->size;
		return retval;
	}
	return NULL;
}

int stringQueueIsEmpty(stringQueue_t *q) {
	return q->head == q->tail;
}

int stringQueueIsFull(stringQueue_t *q) {
	return ((q->head + 1) % q->size) == q->tail;
}

void stringQueueDestroy(stringQueue_t *q) {
	free(q->data);
}