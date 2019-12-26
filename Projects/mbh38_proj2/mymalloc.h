#ifndef _MYMALLOC_H_
#define _MYMALLOC_H_

typedef struct header{
	int dataSize;
	int isFree;
	struct header* next;
	struct header* prev;
} header;

void* my_malloc(unsigned int size);
void my_free(void* ptr);
void printHeaders();
void* expandHeap(unsigned int bytes);

#endif
