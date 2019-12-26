#include <stdio.h>
#include <unistd.h>

#include "mymalloc.h"

int main() {
	// You can use sbrk(0) to get the current position of the break.
	// This is nice for testing cause you can see if the heap is the same size
	// before and after your tests, like here.
	void* heap_at_start = sbrk(0);

	int* arr1 = my_malloc(200*sizeof(int));
	int* arr2 = my_malloc(200*sizeof(int));
	int* arr3 = my_malloc(200*sizeof(int));
	int* arr4 = my_malloc(200*sizeof(int));
	int* arr5 = my_malloc(200*sizeof(int));
	int* arr6 = my_malloc(200*sizeof(int));
	int* arr7 = my_malloc(200*sizeof(int));
	int* arr8 = my_malloc(200*sizeof(int));
	int* arr9 = my_malloc(200*sizeof(int));
	int* arr10 = my_malloc(200*sizeof(int));
	int* arr11 = my_malloc(200*sizeof(int));
	int* arr12 = my_malloc(200*sizeof(int));
	printHeaders();

	my_free(arr1);
	my_free(arr2);
	my_free(arr3);
	my_free(arr4);
	int* arr13 = my_malloc(6);
	int* arr14 = my_malloc(12);
	int* arr15 = my_malloc(24);
	(void)arr13;
	(void)arr14;
	(void)arr15;
	my_free(arr5);
	my_free(arr6);
	my_free(arr7);
	my_free(arr8);
	my_free(arr9);
	my_free(arr10);
	my_free(arr11);
	my_free(arr12);
	my_free(arr13);
	my_free(arr14);
	my_free(arr15);

	printHeaders();
	printf("%p", sbrk(0));






	void* heap_at_end = sbrk(0);
	unsigned int heap_size_diff = (unsigned int)(heap_at_end - heap_at_start);

	if(heap_size_diff)
		printf("Hmm, the heap got bigger by %u (0x%X) bytes...\n", heap_size_diff, heap_size_diff);

	// ADD MORE TESTS HERE.

	return 0;
}
