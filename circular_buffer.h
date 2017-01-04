/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CIRCULAR_BUFFER_H
#define __CIRCULAR_BUFFER_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Implementation based on http://www.embeddedrelated.com/usenet/embedded/show/77084-1.php */
/*************** How to use *****************
 * Buffer size **MUST** be power of 2
 *
 * static uint8_t buffer[512];
 * static CircularBuffer circularBuffer;
 * circularBufferInit(&circularBuffer, buffer, 512);
********************************************/

/* Exported types ------------------------------------------------------------*/
typedef struct{
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    uint32_t mask;
	uint8_t* data;
}CircularBuffer;


/* GNU C Library (glibc) malloc.c isPowerOfTwo implementation */
static inline bool isPowerOfTwo (unsigned int x)
{
  return ((x != 0) && !(x & (x - 1)));
}

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

static inline void circularBufferInit(CircularBuffer *queue, uint8_t *buffer, uint32_t size){
	if (!isPowerOfTwo(size)){
		printf("[circular_buffer]circularBufferInit: Cannot use non power of two size. Halting system\n");
		/* Halt system */
		while(1);
	}

	queue->head = queue->tail = 0;
	queue->data = buffer;
	queue->size = size;
	queue->mask = (queue->size -1);
}

static inline void circularBufferClear(CircularBuffer *queue){
	queue->head = queue->tail = 0;
}

static inline void circularBufferPut(CircularBuffer *queue, uint8_t item){
	queue->data[queue->tail] = item;
	queue->tail = (queue->tail+1) & queue->mask;
}

static inline void circularBufferPutMultiple(CircularBuffer *queue, const uint8_t *item, uint32_t numItem){

	if((queue->tail+numItem)==((queue->tail+numItem) & queue->mask)) {
		memcpy(queue->data+queue->tail, item, numItem);
	}
	else{
                uint32_t lenToTheEnd = queue->size-queue->tail;
		/* Copy from tail up to the end */
		memcpy(queue->data+queue->tail, item, lenToTheEnd);

		/* Copy from beginning until remaining len */
		memcpy(queue->data, item+lenToTheEnd, numItem-lenToTheEnd);
	}

	queue->tail = (queue->tail+numItem) & queue->mask;
}

static inline uint8_t circularBufferGet(CircularBuffer *queue){
	uint8_t item = queue->data[queue->head];
	queue->head = (queue->head+1) & queue->mask;
	return item;
}

static inline void circularBufferDiscardMultiple(CircularBuffer *queue, uint32_t numItem){
	queue->head = (queue->head+numItem) & queue->mask;
}

static inline uint8_t circularBufferHead(CircularBuffer *queue){
	return queue->data[queue->head];
}

static inline bool circularBufferEmpty(CircularBuffer *queue){
	return (queue->head == queue->tail);
}

static inline bool circularBufferFull(CircularBuffer *queue){
	return (((queue->tail+1) & queue->mask) == queue->head);
}

static inline uint32_t circularBufferUsedElementsNum(const CircularBuffer *queue){
	if (queue->head > queue->tail){
		return queue->size - queue->head + queue->tail;
	}
	else{
		return queue->tail - queue->head;
	}
}

static inline uint32_t circularBufferFreeElementsNum(CircularBuffer *queue){
	return queue->size -1 -circularBufferUsedElementsNum(queue);
}

static inline uint8_t circularBufferPeek(CircularBuffer *queue, uint32_t elementPosition){
	return queue->data[(queue->head+elementPosition) & queue->mask];
}

static inline void circularBufferPeekMultiple(const CircularBuffer *queue, uint8_t *dest, const uint32_t len){
	/* If continuous in memory */
	if ((queue->head+len)==((queue->head+len) & queue->mask)){
		memcpy(dest, queue->data+queue->head, len);
	}
	/* If not continuous in memory */
	else{
            uint32_t lenToTheEnd = queue->size-queue->head;
            /* Copy from head up to the end */
            memcpy(dest, queue->data+queue->head, lenToTheEnd);
            /* Copy from beginning until remaining len */
            memcpy(dest + lenToTheEnd, queue->data, len-lenToTheEnd);
	}
}

static inline void circularBufferPeekFromEndMultiple(const CircularBuffer *queue, uint8_t *dest, const uint32_t len){
    /* If continuous in memory */
    if ((queue->tail-len)==((queue->tail-len) & queue->mask)){
        memcpy(dest, queue->data+queue->tail-len, len);
    }
    /* If not continuous in memory */
    else{
            uint32_t lenToTheEnd = len-queue->tail;
            //printf("lenToTheEnd %d , size %d, tail %d\n",lenToTheEnd,queue->size,queue->tail);
            /* Copy from head up to the end */
            memcpy(dest, queue->data+queue->size-lenToTheEnd, lenToTheEnd);
            /* Copy from beginning until remaining len */
            memcpy(dest + lenToTheEnd, queue->data, len-lenToTheEnd);
    }
}

static inline void circularBufferPeekMultipleAndPutMultiple(const CircularBuffer *srcQueue, CircularBuffer *destQueue, const uint32_t len){
    /* If continuous in memory */
    if ((srcQueue->head+len)==((srcQueue->head+len) & srcQueue->mask)){
            circularBufferPutMultiple(destQueue, srcQueue->data+srcQueue->head, len);
    }
    /* If not continuous in memory */
    else{
            uint32_t lenToTheEnd = srcQueue->size-srcQueue->head;
            /* Copy from head up to the end */
            circularBufferPutMultiple(destQueue, srcQueue->data+srcQueue->head, lenToTheEnd);
            /* Copy from beginning until remaining len */
            circularBufferPutMultiple(destQueue, srcQueue->data, len-lenToTheEnd);
    }
}

static inline void circularBufferGetMultiple(CircularBuffer *queue, uint8_t *dest, uint32_t numItem){
    circularBufferPeekMultiple(queue, dest, numItem);
    queue->head = (queue->head+numItem) & queue->mask;
}


/*static char none[8];
static char buffer2[8];
static CircularBuffer circularBuffer2;
circularBufferInit(&circularBuffer2, buffer2, 8);

circularBufferPutMultiple(&circularBuffer2,"1234",4);
circularBufferGetMultiple(&circularBuffer2,none,4);
circularBufferPutMultiple(&circularBuffer2,"56789",5);

circularBufferPeekMultiple(&circularBuffer2,none,5);
none[5]='\0';
printf(none);

circularBufferInit(&circularBuffer2, buffer2, 8);
circularBufferPutMultiple(&circularBuffer2,"1234",4);
circularBufferPeekFromEndMultiple(&circularBuffer2,none,2);
none[2]='\0';
printf("%s\n");
circularBufferGetMultiple(&circularBuffer2,none,4);
none[4]='\0';
printf("%s\n");

circularBufferPutMultiple(&circularBuffer2,"123456",6);
circularBufferPeekFromEndMultiple(&circularBuffer2,none,5);
none[5]='\0';
printf("%s\n");

circularBufferPeekMultiple(&circularBuffer2,none,5);
none[5]='\0';
printf("%s\n");*/

#endif /* __CIRCULAR_BUFFER_H */
