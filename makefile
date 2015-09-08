all:
	nasm -f elf32 kernel/kernel.asm -o kernel/kasm.o
	gcc -m32 -c kernel/kernel.c -o kernel/kc.o
	ld -m elf_i386 -T kernel/link.ld -o OSBS-001 kernel/kasm.o kernel/kc.o
	rm kernel/*.o
