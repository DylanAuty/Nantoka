;;kernel.asm

bits 32
section .text

global start
extern kmain	;kmain is a c function defined elsewhere

start:
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

	cli						; Disable interrupts
	mov esp, stack_space	; stack_space is address of top of stack allocated in .bss. Intel X86 asm puts target first (in this case the stack pointer - there's a separate one for the bottom of the stack too, ebp)
	call kmain				; Call the kmain() function in kernel.c
	hlt						; If we come back from kmain, then halt the processor - cli earlier ensures it properly stops.
	

section .bss
resb 8192		; Set aside 8kb for the stack, resb is REServe memory in Bytes
stack_space:
