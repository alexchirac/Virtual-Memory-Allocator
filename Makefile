CC=gcc
CFLAGS=-Wall -Wextra -std=c99

TARGETS=vma

build: $(TARGETS)

vma: main.c vma.c
	$(CC) $(CFLAGS) *.c -o vma

run_vma:
	./vma

pack:
	zip -FSR grupa_nume_Tema1.zip README Makefile *.c *.h

clean:
	rm -f $(TARGETS)

.PHONY: pack clean