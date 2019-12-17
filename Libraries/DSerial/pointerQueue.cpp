#include "pointerQueue.h"

int pointerQueueInit(pointerQueue_t *q, uint8_t size) {
	size = size + 1; // leave room for buffer NULL item
	void *ptr = malloc(size * sizeof(char*));
	if(ptr) {
		q->data = (char **)ptr;
		q->size = size;
		q->head = 0;
		q->tail = 0;
		for (int i = 0; i < q->size; ++i) {
			q->data[i] = NULL;
		}
		return 1;
	}
	return 0;
}

int pointerQueueAdd(pointerQueue_t *q, char* s) {
	if(!pointerQueueIsFull(q)) {
		q->data[q->head] = s;
		q->head = (q->head + 1) % q->size;
		return 1;
	}
	return 0;
}

char *pointerQueueRemove(pointerQueue_t *q) {
	char *retval;
	if(!pointerQueueIsEmpty(q)) {
		retval = q->data[q->tail];
		q->data[q->tail] = NULL;
		q->tail = (q->tail + 1) % q->size;
		return retval;
	}
	return NULL;
}

int pointerQueueIsEmpty(pointerQueue_t *q) {
	return q->head == q->tail;
}

int pointerQueueIsFull(pointerQueue_t *q) {
	return ((q->head + 1) % q->size) == q->tail;
}

void pointerQueueDestroy(pointerQueue_t *q) {
	free(q->data);
}

void pointerQueuePrint(pointerQueue_t *q) {
	printf("Size: %d, Head: %d, Tail: %d\n", q->size, q->head, q->tail);
	for (int i = 0; i < q->size; ++i)
	{
		if(i == q->head && i == q->tail){
			printf("B->");
		} else if(i == q->head){
			printf("H->");
		} else if (i == q->tail) {
			printf("T->");
		} else {
			printf("   ");
		}
		if(q->data[i] != NULL){
			printf("%d: %p\n", i, q->data[i]);
		} else {
			printf("%d: NULL\n", i);
		}
	}
	printf("\n");
}