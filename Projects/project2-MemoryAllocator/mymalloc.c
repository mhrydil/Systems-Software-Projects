#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mymalloc.h"

#define PTR_ADD_BYTES(ptr, byte_offs) ((void*)(((char*)(ptr)) + (byte_offs)))

// Don't change or remove these constants.
#define MINIMUM_ALLOCATION  16
#define SIZE_MULTIPLE       8

header* heapHead;
header* heapTail;


unsigned int round_up_size(unsigned int data_size) {
	if(data_size == 0)
		return 0;
	else if(data_size < MINIMUM_ALLOCATION)
		return MINIMUM_ALLOCATION;
	else
		return (data_size + (SIZE_MULTIPLE - 1)) & ~(SIZE_MULTIPLE - 1);
}

header createHeader(int size, int free, header* n, header* p){
	header temp = {size, free, n, p};
	return temp;
}

int headIsNull(){
	return (heapHead == NULL);
}

//increases the break with enough space for the size of one header + the number of bytes the user wants to allocate
void* initHeap(unsigned int size){
	void* front = sbrk(size + sizeof(header));
	return front;
}

void* expandHeap(unsigned int bytes){
	void* ptr = sbrk(bytes);
	return ptr;
}

void* contractHeap(unsigned int bytes){
	void* ptr = sbrk(-bytes);
	return ptr;
}

// prints the address and size of each of the headers
// for debugging purposes
void printHeaders(){
	printf("Printing Headers:\n");
	printf("heapHead: %p\n", heapHead);
	printf("heapTail: %p\n", heapTail);
	for(header* curr = heapHead; curr != NULL; curr = curr->next){
		printf("address: %p size: %d free: %d next: %p prev: %p --->     ", curr, curr->dataSize, curr->isFree, curr->next, curr->prev);
	}
	printf("\n");
}


header* findWorstBlock(){
	// printHeaders();
	int currSize = 0;
	
	header* curr = heapHead;
	header* worstFit = curr;

	while(curr != heapTail){
		if(curr->isFree && curr->dataSize > currSize){
			currSize = curr->dataSize;
			worstFit = curr;
		}
		curr = curr->next;
	}
	if(worstFit == NULL) return NULL;
	if(!worstFit->isFree) return NULL; //there were no free blocks
	else return worstFit; //we have a free block (may or may not be large enough)
}

void splitBlock(header* largestFreeBlock, unsigned int size){
	unsigned int currSize = largestFreeBlock->dataSize;
	header* newHeader = PTR_ADD_BYTES(largestFreeBlock, sizeof(header)+size);
	*newHeader = createHeader(currSize-sizeof(header)-size, 1, largestFreeBlock->next, largestFreeBlock);
	largestFreeBlock->next = newHeader;
	largestFreeBlock->dataSize = size;
	largestFreeBlock->isFree = 0;
	if(newHeader->next != NULL){
		newHeader->next->prev = newHeader;
	}
	if(heapTail == largestFreeBlock){
		heapTail = newHeader;
	}
}



void* my_malloc(unsigned int size) {
	if(size == 0)
		return NULL;
	size = round_up_size(size);
	// ------- Don't remove anything above this line. -------
	// Here's where you'll put your code!

	// if headIsNull, the heap hasn't been initialized yet, or it has been made empty through freeing
	if(headIsNull()){
		void* front = initHeap(size);
		heapHead = front; //front is the address of the start of the heap
		*heapHead = createHeader(size, 0, NULL, NULL); //add a header to the value at heapHead
		heapTail = front;
		return PTR_ADD_BYTES(heapHead, sizeof(header)); // front+sizeof(header);
	}

	// there is something already in the heap, so search for open block or increase size of heap
	else{
		//find the largestFreeBlock
		header* largestFreeBlock = findWorstBlock();

		// there are no free blocks large enough, expand heap, add header, return 
		if(largestFreeBlock == NULL || largestFreeBlock->dataSize < size){
			header* oldEnd = expandHeap(size+sizeof(header)); //there are no large enough free blocks, expand heap
			*oldEnd = createHeader(size, 0, NULL, heapTail);
			heapTail->next = oldEnd;
			heapTail = oldEnd;
			return PTR_ADD_BYTES(oldEnd, sizeof(header));
		}

		//there's a free block large enough for this allocation
		else{
			//if the block can be split into two smaller blocks, do that
			if((largestFreeBlock->dataSize - size) >= MINIMUM_ALLOCATION + sizeof(header)){
				splitBlock(largestFreeBlock, size);
				return PTR_ADD_BYTES(largestFreeBlock, sizeof(header));
			}
			// the block can't be split, just mark it as used and return a pointer to the data
			else{
				largestFreeBlock->isFree = 0; //mark block as used
				return PTR_ADD_BYTES(largestFreeBlock, sizeof(header));
			}
		}
		return largestFreeBlock;
		//if (largestFreeBlock == NULL) return 0;
		
	}
	//return NULL;
}

// coalesces neighboring free blocks and returns the pointer to the start of the newly coalesced block
void* coalesce(void* ptr){
	header* curr = ptr-sizeof(header);
	if(curr->prev != NULL){
		if(curr->prev->isFree){
			header* before = curr->prev;
			before->next = curr->next;
			before->dataSize = before->dataSize + sizeof(header) + curr->dataSize;
			if(curr->next != NULL){
				curr->next->prev = before;
			}
			if(curr == heapTail){
				heapTail = before;
			}
			curr = before;
		}
	}
	if(curr->next != NULL){
		if(curr->next->isFree){
			header* after = curr->next;
			curr->next = after->next;
			curr->dataSize = curr->dataSize + sizeof(header) + after->dataSize;
			if(after == heapTail){
				heapTail = curr;
			}
			if(after->next != NULL){
				after->next->prev = curr;
			}
		}
	}
	return PTR_ADD_BYTES(curr, sizeof(header));

}

//if there is only one block and it is being freed, set head and tail to NULL and contract the heap
void freeOnlyBlock(void* ptr){
	header* curr = ptr - sizeof(header);
	int bytesToDrop = curr->dataSize + sizeof(header);
	heapTail = NULL;
	heapHead = NULL;
	contractHeap(bytesToDrop);
}

//checks to see if the last block is the only block in the list, if not it frees the last block, sets the tail to the correct position and contracts the heap
void freeLastBlock(void* ptr){
	if(heapTail == heapHead){
		freeOnlyBlock(ptr);
	}
	else{
		header* curr = (header*)(ptr - sizeof(header));
		heapTail = heapTail->prev;
		heapTail->next = NULL;
		int bytesToDrop = curr->dataSize + sizeof(header);
		contractHeap(bytesToDrop);
	}
}

//changes the isFree value of a header to true
void freeBlock(void* ptr){
	header* curr = ptr - sizeof(header);
	curr->isFree = 1;
}

//returns true if the pointer lines up with the start of a block.
int pointsToValidBlock(void* ptr){
	header* startOfHeader = ptr - sizeof(header);
	for(header* curr = heapHead; curr != NULL; curr = curr->next){
		if(curr == startOfHeader){
			return 1;
		}
	}
	return 0;
}

void my_free(void* ptr) {
	if(ptr == NULL){
		printf("ptr == NULL");
		return;
	}
	else{

		// ptr is pointing to the last block in the heap, free it and contract the heap
		if(ptr == PTR_ADD_BYTES(heapTail, sizeof(header))){
			ptr = coalesce(ptr);
			freeLastBlock(ptr);
		}
		//pointer is pointing somewhere other than the last block
		else{
			if (!pointsToValidBlock(ptr)) return; //if pointer doesn't point to a valid block, just return
			
			ptr = coalesce(ptr);
			//after coalescing, the pointer could have changed to the tail of the linked list, so check again
			if(ptr == PTR_ADD_BYTES(heapTail, sizeof(header))) freeLastBlock(ptr);
			else freeBlock(ptr);
		}
	}

	// and here's where you free stuff.
}
