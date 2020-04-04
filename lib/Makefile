.DEFAULT_GOAL=runtime.o

%.o: %.S
	nasm -f elf64 $^

clean:
	rm -f *.o >/dev/null 2>/dev/null
