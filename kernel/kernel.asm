;;kernel.asm

bits 32
section .text
	; To make it multiboot compatible, we need to add stuff to the multiboot header
	; The multiboot header must be within the first 8KB of the kernel
	; needs 3 fields, all 4 byte aligned:
	; magic - magic number to identify the header as being multiboot compatible
	; flags - set to 0, we don't care about them
	; checksum - adding to both magic and flags should give zero.
	align 4
	dd 0x1BADB002		; magic number
	dd 0x00		; flags
	dd - (0x1BADB002 + 0x00) ; checksum. m + f + c should be 0. dd declares a 4 byte data location.
	; These 3 dd's have no labels for the values, and are located immediately after each other. Since they're the first things declared,
	; they should be the within the required header.

global start
global keyboard_handler
global read_port
global write_port
global load_idt

extern kmain	;kmain is a c function defined elsewhere
extern keyboard_handler_main

write_port:				# Takes 2 arguments, port number and data to be written. Function 'out' writes to port at given address.
	mov edx, [esp + 4]	# Argument 1 is port number
	mov al, [esp + 8]	# al is lower 16 bits of EAX - for returns from functions but also for arithmetic, interrupt calls, etc...
						# Don't think there's any reason to use eax over others, since we're not trying to return anything
						# Probably just for convenience's sake?
	out dx, al			# Write the argument (stored in al) to the port at dx
	ret

read_port:				# read_port will take the port number as an argument
	mov edx, [esp+4]	# When calling functions with arguments, the arguments get shoved onto the stack
						# Reading them means using the stack pointer to fetch them, and put them in the edx register.
						# At this point we can work with the input argument. Adding 4 because we're at the top of the stack.
	in al, dx			# dx is the lower 16 bits of register edx.
						# in reads the port whose number is in the second argument - in this case dx.
						# The result goes into al. Note this is different to mov, as dx is not the value - it's the value at
						# port number dx that we want.
						# al is the lower 8 bits of register eax - eax is used to provide returns from functions - same as writing
						# 'return value' at the end of a c function call and read as such by a c function.
	ret					# go back wherever this is from

load_idt:
	mov edx, [esp + 4]	# function is passed a pointer to the IDT as an argument, which we retrieve in the standard way
	lidt [edx]	# lidt is a command solely for informing the processor where it can find the idt
	sti	# Command to turn on interrupts, cli being the opposite
	ret

keyboard_handler:
	call keyboard_handler_main	# C code is easier than x86 asm so we're calling a c function here.
	iretd	# Special command to return from interrupt based functions - pops the flags register that was pushed onto the 
			# stack when the interrupt call was made.

start:
	cli						; Disable interrupts
	mov esp, stack_space	; stack_space is address of top of stack allocated in .bss. Intel X86 asm puts target first (in this case the stack pointer - there's a separate one for the bottom of the stack too, ebp)
	call kmain				; Call the kmain() function in kernel.c
	hlt						; If we come back from kmain, then halt the processor - cli earlier ensures it properly stops.
	
section .bss
resb 8192		; Set aside 8kb for the stack, resb is REServe memory in Bytes
stack_space:

