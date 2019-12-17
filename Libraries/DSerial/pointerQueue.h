/** @file pointerQueue.h
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
} pointerQueue_t;

int pointerQueueInit(pointerQueue_t *q, uint8_t size);
int pointerQueueAdd(pointerQueue_t *q, char* s);
char *pointerQueueRemove(pointerQueue_t *q);
int pointerQueueIsEmpty(pointerQueue_t *q);
int pointerQueueIsFull(pointerQueue_t *q);
void pointerQueueDestroy(pointerQueue_t *q);
void pointerQueuePrint(pointerQueue_t *q);
