#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct position position;
struct position {
	int block_pos;
	int miniblock_pos;
	int is_start;
};

/* TODO : add your implementation for doubly-linked list */
typedef struct node node;
struct node {
	void *data;
	node *prev, *next;
};

typedef struct list_t list_t;
struct list_t {
	node *head;
	int size;
	int data_size;
};

typedef struct {
	uint64_t start_address;
	size_t size;
	void *miniblock_list;
} block_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

list_t *dll_create(unsigned int data_size);
node *dll_get_nth_node(list_t *list, int n);
void dll_add_nth_node(list_t *list, int n, const void *new_data);
node *dll_remove_nth_node(list_t *list, int n);
void dll_free(list_t **pp_list);
void dll_print_string_list(list_t *list);
void dll_print_int_list(list_t *list);

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);

void read(arena_t *arena, uint64_t address, uint64_t size);
void
write(arena_t *arena, const uint64_t address,
	  const uint64_t size, int8_t *data);
void pmap(const arena_t *arena);
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);