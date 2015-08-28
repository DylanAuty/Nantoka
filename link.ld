/* link.ld
 */

OUTPUT_FORMAT(elf32-i386)
ENTRY(start)
SECTIONS
{
	. = 0x100000;
	.text : {*(.text)}
	.data : {*(.data)}
	.bss : {*(.bss)}
}

/*
 * OUTPUT_FORMAT to specify we want a 32 bit ELF, for i386 specifically (32 bit)
 * ENTRY defines our entry point as the symbol 'start', defined in kernel.asm.
 * SECTIONS describes the layout of the linked executable - starting at 0x100000, it goes on to link all text sections, then data then bss together, in order, from any provided files.
 * Starts at 0x100000, the bootloader loads the kernel.
 *
 */
