/** @file stringQueue.h
 *  @brief Headers and definitions for a simple FIFO queue of strings.
 *
 *  @author Dillon Lareau (dlareau)
 */
#pragma once
#include "Arduino.h"

typedef struct {
	char **data;
	uint8_t head;
	uint8_t tail;
	uint8_t size;
} stringQueue_t;

int stringQueueInit(stringQueue_t *q, uint8_t size);
int stringQueueAdd(stringQueue_t *q, char* s);
char *stringQueueRemove(stringQueue_t *q);
int stringQueueIsEmpty(stringQueue_t *q);
int stringQueueIsFull(stringQueue_t *q);
void stringQueueDestroy(stringQueue_t *q);