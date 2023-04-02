#include "vma.h"

/*
 * Functie care trebuie apelata pentru alocarea si initializarea unei liste.
 * (Setare valori initiale pentru campurile specifice structurii LinkedList).
 */
list_t*
dll_create(unsigned int data_size)
{
	list_t *list = malloc(sizeof(list_t));
	list->data_size = data_size;
	list->size = 0;
	return list;
}

/*
 * Functia intoarce un pointer la nodul de pe pozitia n din lista.
 * Pozitiile din lista sunt indexate incepand cu 0 (i.e. primul nod din lista se
 * afla pe pozitia n=0). Daca n >= nr_noduri, atunci se intoarce nodul de pe
 * pozitia rezultata daca am "cicla" (posibil de mai multe ori) pe lista si am
 * trece de la ultimul nod, inapoi la primul si am continua de acolo. Cum putem
 * afla pozitia dorita fara sa simulam intreaga parcurgere?
 * Atentie: n>=0 (nu trebuie tratat cazul in care n este negativ).
 */
node*
dll_get_nth_node(list_t *list, int n)
{
	if (n > list->size)
		n = n % list->size;
	node *cur = list->head;
	for (int i = 0; i < n; i++)
		cur = cur->next;
	return cur;
}

/*
 * Pe baza datelor trimise prin pointerul new_data, se creeaza un nou nod care e
 * adaugat pe pozitia n a listei reprezentata de pointerul list. Pozitiile din
 * lista sunt indexate incepand cu 0 (i.e. primul nod din lista se afla pe
 * pozitia n=0). Cand indexam pozitiile nu "ciclam" pe lista circulara ca la
 * get, ci consideram nodurile in ordinea de la head la ultimul (adica acel nod
 * care pointeaza la head ca nod urmator in lista). Daca n >= nr_noduri, atunci
 * adaugam nodul nou la finalul listei.
 * Atentie: n>=0 (nu trebuie tratat cazul in care n este negativ).
 */
void
dll_add_nth_node(list_t *list, int n, const void *new_data)
{
	node *new_node = malloc(sizeof(node));
	new_node->data = malloc(list->data_size);
	memcpy(new_node->data, new_data, list->data_size);
	if (list->size == 0) {
		list->head = new_node;
		new_node->next = new_node;
		new_node->prev = new_node;
		list->size++;
		return;
	}
	if (n == 0) {
		new_node->next = list->head;
		new_node->prev = list->head->prev;
		list->head->prev->next = new_node;
		list->head->prev = new_node;
		list->head = new_node;
		list->size++;
		return;
	}
	if (n > list->size)
		n = list->size;
	node *cur;
	cur = dll_get_nth_node(list, n - 1);

	new_node->next = cur->next;
	new_node->prev = cur;
	cur->next->prev = new_node;
	cur->next = new_node;
	list->size++;
}

/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Functia intoarce un pointer spre acest nod
 * proaspat eliminat din lista. Daca n >= nr_noduri - 1, se elimina nodul de la
 * finalul listei. Este responsabilitatea apelantului sa elibereze memoria
 * acestui nod.
 * Atentie: n>=0 (nu trebuie tratat cazul in care n este negativ).
 */
node*
dll_remove_nth_node(list_t *list, int n)
{
	node *cur;
	if (n > list->size - 1)
		n = list->size - 1;
	cur = dll_get_nth_node(list, n);

	if (n == 0)
		list->head = cur->next;
	cur->next->prev = cur->prev;
	cur->prev->next = cur->next;
	list->size--;

	return cur;
}

/*
 * Procedura elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista.
 */
void
dll_free(list_t **pp_list)
{
	node *cur = (*pp_list)->head->prev;
	node *aux;
	while (cur != (*pp_list)->head) {
		aux = cur->prev;
		free(cur->data);
		free(cur);
		cur = aux;
	}
	free(cur->data);
	free(cur);
	free(*pp_list);
}

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	arena->arena_size = size;
	arena->alloc_list = dll_create(sizeof(block_t));
	return arena;
}

/*
Functie care dealoca si sterge un miniblock
*/
void del_miniblock(miniblock_t *miniblock)
{
	free(miniblock->rw_buffer);
	free(miniblock);
}

/*
Functie care dealoca un block
*/
void del_block(block_t *block)
{
	node *cur, *aux;
	list_t *list = ((list_t *)block->miniblock_list);
	if (list->size != 0) {
		cur = list->head->prev;
		while (cur != list->head) {
			aux = cur->prev;
			miniblock_t *miniblock = (miniblock_t *)cur->data;
			del_miniblock(miniblock);
			free(cur);
			cur = aux;
		}
		miniblock_t *miniblock = (miniblock_t *)cur->data;
		del_miniblock(miniblock);
		free(cur);
	}
	free(list);
	free(block);
}

/*
Functie care concateneaza 2 blocuri
*/
void concatenate(node *node1, node *node2)
{
	block_t *block1 = (block_t *)node1->data;
	block_t *block2 = (block_t *)node2->data;
	list_t *list1 = (list_t *)block1->miniblock_list;
	list_t *list2 = (list_t *)block2->miniblock_list;
	block1->size = block1->size + block2->size;
	list1->size += list2->size;
	node *head1, *head2, *tail1, *tail2;
	head1 = list1->head;
	head2 = list2->head;
	tail1 = list1->head->prev;
	tail2 = list2->head->prev;
	tail1->next = head2;
	head2->prev = tail1;
	tail2->next = head1;
	head1->prev = tail2;
	free(list2);
	free(block2);
	node1->next = node2->next;
	node2->next->prev = node1;
	free(node2);
}

/*
Functie care cauta prin toata arena si daca gaseste
2 blocuri adiacente le combina intr-unul singur
*/
void combine_arena(arena_t *arena)
{
	list_t *list = arena->alloc_list;
	uint64_t address1, size1, address2, addr_fin1;
	block_t *block1, *block2;
	node *cur = list->head;
	do {
		block1 = (block_t *)cur->data;
		block2 = (block_t *)cur->next->data;
		address1 = block1->start_address;
		size1 = block1->size;
		addr_fin1 = address1 + size1;
		address2 = block2->start_address;
		if (addr_fin1 == address2) {
			concatenate(cur, cur->next);
			list->size--;
		} else {
			cur = cur->next;
		}
	} while (cur != list->head->prev);
}

void dealloc_arena(arena_t *arena)
{
	list_t *list = arena->alloc_list;
	if (list->size != 0) {
		node *cur = list->head->prev;
		node *aux;
		while (cur != list->head) {
			aux = cur->prev;
			block_t *block = (block_t *)cur->data;
			del_block(block);
			free(cur);
			cur = aux;
		}
		block_t *block = (block_t *)cur->data;
		del_block(block);
		free(cur);
	}
	free(list);
	free(arena);
}

position get_addr_pos(arena_t *arena, uint64_t address)
{
	list_t *block_list = arena->alloc_list;
	position pos;
	block_t *block;
	miniblock_t *miniblock;
	int i;
	pos.block_pos = -1;
	pos.miniblock_pos = -1;
	pos.is_start = 0;
	if (block_list->size == 0)
		return pos;

	node *cur1 = block_list->head;
	for (i = 0; i < block_list->size; i++) {
		block = (block_t *)cur1->data;
		uint64_t addr_fin = block->start_address + block->size;
		if (address >= block->start_address && address < addr_fin) {
			pos.block_pos = i;
			break;
		}
		cur1 = cur1->next;
	}

	if (i == block_list->size)
		return pos;

	list_t *miniblock_list = (list_t *)block->miniblock_list;
	node *cur2 = miniblock_list->head;
	for (i = 0; i < miniblock_list->size; i++) {
		miniblock = (miniblock_t *)cur2->data;
		uint64_t addr_fin = miniblock->start_address + miniblock->size;
		if (address >= miniblock->start_address && address < addr_fin) {
			pos.miniblock_pos = i;
			if (address == miniblock->start_address)
				pos.is_start = 1;
			return pos;
		}
		cur2 = cur2->next;
	}

	return pos;
}

void free_middle_miniblock(arena_t *arena, position pos)
{
	list_t *block_list = arena->alloc_list;
	node *cur1 = dll_get_nth_node(block_list, pos.block_pos);
	block_t *block = (block_t *)cur1->data;
	list_t *miniblock_list = (list_t *)block->miniblock_list;
	node *cur2 = dll_get_nth_node(miniblock_list, pos.miniblock_pos);

	uint64_t block2_size = 0;
	uint64_t block2_start_adress;

	node *cop1 = cur2;
	list_t *list2 = dll_create(sizeof(miniblock_t));
	cur2 = cur2->next;
	block2_start_adress = ((miniblock_t *)cur2->data)->start_address;
	list2->head = cur2;
	cur2->prev = miniblock_list->head->prev;
	miniblock_list->head->prev->next = cur2;
	list2->size = miniblock_list->size - pos.miniblock_pos - 1;
	do {
		block2_size += ((miniblock_t *)cur2->data)->size;
		cur2 = cur2->next;
	} while (cur2 != list2->head);
	block_t *new_block = malloc(sizeof(block_t));
	new_block->miniblock_list = list2;
	new_block->start_address = block2_start_adress;
	new_block->size = block2_size;
	dll_add_nth_node(block_list, pos.block_pos + 1, new_block);
	free(new_block);

	cur2 = cop1->prev;
	miniblock_list->head->prev = cur2;
	cur2->next = miniblock_list->head;
	miniblock_list->size = pos.miniblock_pos;
	uint64_t deleted_block_size = ((miniblock_t *)cop1->data)->size;
	block->size = block->size - block2_size - deleted_block_size;

	del_miniblock(cop1->data);
	free(cop1);
}

void free_block(arena_t *arena, const uint64_t address)
{
	position pos = get_addr_pos(arena, address);
	if (pos.is_start == 0) {
		printf("Invalid address for free.\n");
		return;
	}
	list_t *block_list = arena->alloc_list;
	node *cur1 = dll_get_nth_node(block_list, pos.block_pos);
	block_t *block = (block_t *)cur1->data;
	list_t *miniblock_list = (list_t *)block->miniblock_list;
	node *cur2 = dll_get_nth_node(miniblock_list, pos.miniblock_pos);
	if (miniblock_list->size == 1) {
		del_block(block);
		cur1 = dll_remove_nth_node(block_list, pos.block_pos);
		free(cur1);
		return;
	}
	if (pos.miniblock_pos == 0) {
		miniblock_t *miniblock = (miniblock_t *)cur2->next->data;
		block->start_address = miniblock->start_address;
		miniblock = (miniblock_t *)cur2->data;
		block->size -= miniblock->size;
		del_miniblock(miniblock);
		cur2 = dll_remove_nth_node(miniblock_list, 0);
		free(cur2);
		return;
	}
	if (pos.miniblock_pos == miniblock_list->size - 1) {
		miniblock_t *miniblock = (miniblock_t *)cur2->data;
		block->size -= miniblock->size;
		del_miniblock(miniblock);
		cur2 = dll_remove_nth_node(miniblock_list, pos.miniblock_pos);
		free(cur2);
		return;
	}
	free_middle_miniblock(arena, pos);
}

/*
Functie ce creeaza un nod nou
*/
node *make_node(const void *data, const int data_size)
{
	node *new_node = malloc(sizeof(node));
	new_node->data = malloc(data_size);
	memcpy(new_node->data, data, data_size);

	return new_node;
}

/*
Functie ce creeaza un miniblock nou si aloca rw_buffer-ul din acesta
*/
miniblock_t *make_miniblock(uint64_t address, size_t size)
{
	miniblock_t *miniblock = malloc(sizeof(miniblock_t));
	miniblock->start_address = address;
	miniblock->size = size;
	miniblock->perm = 6;
	miniblock->rw_buffer = malloc(1);
	return miniblock;
}

/*
Functia creeaza un block nou si insereaza in acesta un singur miniblock
ce ocupa tot spatiul block-ului
*/
block_t *make_block(const uint64_t address, const size_t size)
{
	block_t *block = malloc(sizeof(block_t));
	block->size = size;
	block->start_address = address;
	block->miniblock_list = dll_create(sizeof(miniblock_t));
	list_t *list = (list_t *)block->miniblock_list;
	miniblock_t *miniblock = make_miniblock(address, size);
	dll_add_nth_node(list, 0, miniblock);
	free(miniblock);
	return block;
}

/*
Functia returneaza pozitia unde trebuie inserat noul block la
apelarea functiei "alloc_block". Mai apoi rezultatul aceastei functii
va fi folosit ca parametru in "dll_add_nth_node".
*/
int get_poz(arena_t *arena, uint64_t address, uint64_t size)
{
	list_t *list = arena->alloc_list;
	node *cur = list->head;
	block_t *block1, *block2;
	uint64_t addr_fin = address + size;
	uint64_t addr_fin1;
	int n = 0;
	do {
		if (n == 0) {
			block1 = (block_t *)cur->data;
			if (block1->start_address >= addr_fin)
				return n;
			n++;
		} else if (n == list->size) {
			block1 = (block_t *)cur->data;
			addr_fin1 = block1->start_address + block1->size;
			if (addr_fin1 <= address)
				return n;
			n++;
		} else {
			block1 = (block_t *)cur->data;
			addr_fin1 = block1->start_address + block1->size;
			block2 = (block_t *)cur->next->data;
			if (addr_fin1 <= address && addr_fin <= block2->start_address)
				return n;
			cur = cur->next;
			n++;
		}
	} while (n <= list->size);
	// if (n > list->size)
	return -1;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	list_t *list = arena->alloc_list;
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return;
	}
	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return;
	}
	if (list->size == 0) {
		block_t *block = make_block(address, size);
		dll_add_nth_node(list, 0, block);
		free(block);
		return;
	}
	int n = get_poz(arena, address, size);
	if (n != -1) {
		block_t *block = make_block(address, size);
		dll_add_nth_node(list, n, block);
		free(block);
		combine_arena(arena);
		return;
	}
	printf("This zone was already allocated.\n");
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	position pos = get_addr_pos(arena, address);
	uint64_t read_bytes;
	if (pos.miniblock_pos == -1) {
		printf("Invalid address for read.\n");
		return;
	}
	list_t *block_list = arena->alloc_list;
	node *cur1 = dll_get_nth_node(block_list, pos.block_pos), *cur2;
	block_t *block = (block_t *)cur1->data;
	list_t *miniblock_list = (list_t *)block->miniblock_list;
	miniblock_t *miniblock;
	int n = pos.miniblock_pos;
	cur2 = dll_get_nth_node(miniblock_list, n);
	if (address + size > block->size + block->start_address) {
		for (int i = n; i < miniblock_list->size; i++) {
			miniblock = (miniblock_t *)cur2->data;
			if (miniblock->perm / 4 == 0) {
				printf("Invalid permissions for read.\n");
				return;
			}
			cur2 = cur2->next;
		}
	} else {
		position fin_pos = get_addr_pos(arena, address + size - 1);
		int m = fin_pos.miniblock_pos;
		for (int i = n; i <= m; i++) {
			miniblock = (miniblock_t *)cur2->data;
			if (miniblock->perm / 4 == 0) {
				printf("Invalid permissions for read.\n");
				return;
			}
			cur2 = cur2->next;
		}
	}
	if (address + size > block->size + block->start_address) {
		read_bytes = block->size + block->start_address - address;
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %ld characters.\n", read_bytes);
	} else {
		read_bytes = size;
	}
	miniblock = (miniblock_t *)cur2->data;
	int k = address - miniblock->start_address;
	for (int i = 0; i < read_bytes; i++) {
		if (k == miniblock->size) {
			k = 0;
			n++;
			cur2 = dll_get_nth_node(miniblock_list, n);
			miniblock = (miniblock_t *)cur2->data;
		}
		char c = ((char *)miniblock->rw_buffer)[k];
		printf("%c", c);
		k++;
	}
	printf("\n");
}

void
write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data)
{
	position pos = get_addr_pos(arena, address);
	if (pos.miniblock_pos == -1) {
		printf("Invalid address for write.\n");
		return;
	}
	list_t *block_list = arena->alloc_list;
	node *cur1 = dll_get_nth_node(block_list, pos.block_pos), *cur2;
	block_t *block = (block_t *)cur1->data;
	list_t *miniblock_list = (list_t *)block->miniblock_list;
	uint64_t written_bytes;
	miniblock_t *miniblock;
	int n = pos.miniblock_pos;
	cur2 = dll_get_nth_node(miniblock_list, n);
	if (address + size > block->size + block->start_address) {
		for (int i = n; i < miniblock_list->size; i++) {
			miniblock = (miniblock_t *)cur2->data;
			if (miniblock->perm % 4 / 2 == 0) {
				printf("Invalid permissions for write.\n");
				return;
			}
			cur2 = cur2->next;
		}
	} else {
		position fin_pos = get_addr_pos(arena, address + size - 1);
		int m = fin_pos.miniblock_pos;
		for (int i = n; i <= m; i++) {
			miniblock = (miniblock_t *)cur2->data;
			if (miniblock->perm % 4 / 2 == 0) {
				printf("Invalid permissions for write.\n");
				return;
			}
			cur2 = cur2->next;
		}
	}
	if (address + size > block->size + block->start_address) {
		written_bytes = block->size + block->start_address - address;
		printf("Warning: size was bigger than the block size. ");
		printf("Writing %ld characters.\n", written_bytes);
	} else {
		written_bytes = size;
	}
	cur2 = dll_get_nth_node(miniblock_list, n);
	miniblock = (miniblock_t *)cur2->data;
	free(miniblock->rw_buffer);
	int k = address - miniblock->start_address;
	miniblock->rw_buffer = malloc(miniblock->size);
	for (int i = 0; i < written_bytes; i++) {
		if (k == miniblock->size) {
			k = 0;
			n++;
			cur2 = dll_get_nth_node(miniblock_list, n);
			miniblock = (miniblock_t *)cur2->data;
		}
		((uint8_t *)miniblock->rw_buffer)[k] = data[i];
		k++;
	}
}

char *write_perm(uint8_t perm)
{
	char *char_perm = malloc(4 * sizeof(char));
	if (perm / 4)
		char_perm[0] = 'R';
	else
		char_perm[0] = '-';

	if (perm % 4 / 2)
		char_perm[1] = 'W';
	else
		char_perm[1] = '-';

	if (perm % 2)
		char_perm[2] = 'X';
	else
		char_perm[2] = '-';
	char_perm[3] = '\0';

	return char_perm;
}

void print_block(block_t *block, int indx)
{
	printf("Block %d begin\n", indx);
	uint64_t addr_fin = block->start_address + block->size;
	printf("Zone: 0x%lX - 0x%lX\n", block->start_address, addr_fin);
	list_t *list = (list_t *)block->miniblock_list;
	node *cur = list->head;
	for (int i = 0; i < list->size; i++) {
		miniblock_t *miniblock = (miniblock_t *)cur->data;
		uint64_t addr_fin = miniblock->start_address + miniblock->size;
		char *char_perm = write_perm(miniblock->perm);
		printf("Miniblock %d:\t\t0x%lX\t\t", i + 1, miniblock->start_address);
		printf("-\t\t0x%lX\t\t| %s\n", addr_fin, char_perm);
		free(char_perm);
		cur = cur->next;
	}
	printf("Block %d end\n", indx);
}

void pmap(const arena_t *arena)
{
	uint64_t free_mem, taken_memory = 0;
	int nr_miniblocks = 0;
	list_t *list = arena->alloc_list, *mini_list;
	node *cur;
	if (list->size)
		cur = list->head;
	for (int i = 0; i < list->size; i++) {
		block_t *block = (block_t *)cur->data;
		taken_memory += block->size;
		mini_list = (list_t *)block->miniblock_list;
		nr_miniblocks += mini_list->size;
		cur = cur->next;
	}
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	free_mem = arena->arena_size - taken_memory;
	printf("Free memory: 0x%lX bytes\n", free_mem);
	printf("Number of allocated blocks: %d\n", list->size);
	printf("Number of allocated miniblocks: %d\n", nr_miniblocks);
	for (int i = 0; i < list->size; i++) {
		printf("\n");
		block_t *block = (block_t *)cur->data;
		print_block(block, i + 1);
		cur = cur->next;
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	position pos = get_addr_pos(arena, address);
	if (pos.is_start == 0) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	char p[7][11];
	int nr = sscanf(permission, "%s %s %s %s %s %s %s",
					p[0], p[1], p[2], p[3], p[4], p[5], p[6]);

	list_t *block_list = arena->alloc_list;
	node *cur1 = dll_get_nth_node(block_list, pos.block_pos);
	block_t *block = (block_t *)cur1->data;
	list_t *miniblock_list = (list_t *)block->miniblock_list;
	node *cur2 = dll_get_nth_node(miniblock_list, pos.miniblock_pos);
	miniblock_t *miniblock = (miniblock_t *)cur2->data;
	int perm_vector[4] = {0, 0, 0};
	for (int i = 0; i < nr; i = i + 2) {
		if (strncmp(p[i], "PROT_NONE", 9) == 0) {
			perm_vector[0] = 0;
			perm_vector[1] = 0;
			perm_vector[2] = 0;
			break;
		}
		if (strncmp(p[i], "PROT_READ", 9) == 0)
			perm_vector[0] = 1;

		if (strncmp(p[i], "PROT_WRITE", 10) == 0)
			perm_vector[1] = 1;

		if (strncmp(p[i], "PROT_EXEC", 9) == 0)
			perm_vector[2] = 1;
	}
	int new_perm = 0, cnt = 1;
	for (int i = 2; i >= 0; i--) {
		new_perm = new_perm + perm_vector[i] * cnt;
		cnt = cnt * 2;
	}
	miniblock->perm = new_perm;
}
