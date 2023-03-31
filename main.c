#include "vma.h"

#define MAX_STRING_SIZE 64

int main(void)
{
	char com[100];
	uint64_t size, address;
	arena_t *arena;
	while (1) {
		scanf("%s", com);
		if (strncmp(com, "ALLOC_ARENA", 11) == 0) {
			scanf("%ld", &size);
			arena = alloc_arena(size);
			continue;
		}
		if (strncmp(com, "ALLOC_BLOCK", 11) == 0) {
			scanf("%ld %ld", &address, &size);
			alloc_block(arena, address, size);
			continue;
		}
		if (strncmp(com, "FREE_BLOCK", 10) == 0) {
			scanf("%ld", &address);
			free_block(arena, address);
			continue;
		}
		if (strncmp(com, "PMAP", 4) == 0) {
			pmap(arena);
			continue;
		}
		if (strncmp(com, "DEALLOC_ARENA", 13) == 0) {
			dealloc_arena(arena);
			break;
		}
	}
	return 0;
}