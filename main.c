#include "vma.h"

#define MAX_STRING_SIZE 64

int main(void)
{
	char com[100];
	uint64_t size, address;
	arena_t *arena;
	char *s;
	char c, permission[50];
	int j = 0;
	while (j < 5000) {
		scanf("%s", com);
		if (strcmp(com, "ALLOC_ARENA") == 0) {
			scanf("%ld", &size);
			arena = alloc_arena(size);
			continue;
		}
		if (strcmp(com, "ALLOC_BLOCK") == 0) {
			scanf("%ld %ld", &address, &size);
			alloc_block(arena, address, size);
			continue;
		}
		if (strcmp(com, "FREE_BLOCK") == 0) {
			scanf("%ld", &address);
			free_block(arena, address);
			continue;
		}
		if (strcmp(com, "PMAP") == 0) {
			pmap(arena);
			continue;
		}
		if (strcmp(com, "WRITE") == 0) {
			scanf("%ld %ld", &address, &size);
			s = malloc(size);
			for (int i = 0; i < size; i++) {
				s[i] = getc(stdin);
				if (i == 0 && s[i] == ' ')
					s[i] = getc(stdin);
				if (s[i] == '\0')
					s[i] = '\n';
			}
			write(arena, address, size, (uint8_t *)s);
			continue;
		}
		if (strcmp(com, "READ") == 0) {
			scanf("%ld %ld", &address, &size);
			read(arena, address, size);
			continue;
		}
		if (strcmp(com, "DEALLOC_ARENA") == 0) {
			dealloc_arena(arena);
			break;
		}
		if (strcmp(com, "MPROTECT") == 0) {
			scanf("%ld", &address);
			fgets(permission, 50, stdin);
			mprotect(arena, address, permission);
			continue;
		}
		printf("Invalid command. Please try again.\n");
		j++;
		continue;
	}
	return 0;
}
