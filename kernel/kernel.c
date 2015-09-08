/* kernel.c
 *
 */

// Global kludgefixes because of laziness

char *vidptr = (char*)0xb8000;		/* Start of relevant bit of video memory
									 * In REAL mode, video memory of VGA is mapped to PC memory between segments 0xA0000 and 0xBFFFF
									 * This breaks down as:
									 * 0xA0000 for EGA/VGA graphics modes (64KB)
									 * 0xB0000 for monochrome text mode (32KB)
									 * 0xB8000 for colour text mode and CGA compatible graphics modes.
									 */
unsigned int current_loc = 0;		// Used as a cursor... gotta look up interrupt function scope




struct IDT_entry {	// Interrupt descriptor table structure - gives the interrupt handler address corresponding to
					// the interrupt number
					// Keyboard sends an interrupt request along its interrupt line to the programmable interrupt controller (PIC).
					// The IO ports for the keyboard are 0x60 and 0x64, for data and status respectively.
					unsigned short int offset_lowerbits;
					unsigned short int selector;
					unsigned char zero;
					unsigned char type_attr;
					unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void kmain(void){	//kmain was called in kernel.asm; returning from it halts the processor
	// At the moment the aim is to clear the screen and write some text to it
	idt_init();	// Call function to initialise interrupts for the keyboard.
	kb_init();	// Call function that uses mask on PIC port to turn interrupt on for keyboard only
	
	const char *str = "Nantoka v0.0.0";	// String to write to screen
	
	// Loop to clear the screen - 25 lines of 80 columns each. Each element takes 2 bytes, one for ASCII code and one for formatting
	unsigned int j = 0;
	while(current_loc < (80 * 25 * 2)){
		vidptr[current_loc] = ' ';	// Replace whatever it is with a blank character
		vidptr[current_loc + 1] = 0x07;	// Formatting/attribute byte - light grey on black, refer to chart for complete list
		current_loc = current_loc + 2;	// Jump to next character's pair of bytes
	}

	current_loc = 0;	// Should be here anyway but just in case it isn't...
	j = 0;	// Now i and j are back to 0

	// Now write the string over the top of the blank screen
	while(str[j] != '\0') {	// Until the eol character...
		vidptr[current_loc] = str[j];		// Write ASCII byte
		vidptr[current_loc + 1] = 0x07;		// 0x07 in attribute byte means light grey (the 7) on black background (0).
		j++;					// Step to next character to be written
		current_loc = current_loc + 2;				// Step to next space in screen memory to be written to.
	}
	return;
}

void idt_init(void) {
	// Function to initialise the PICs and the IDT
	
	// First initialise the 2 PICs in the system - most X86 systems have 2 PICs
	
	/*	PIC PORTS
	 *			PIC1	PIC2
	 *	Command	0x20	0xA0
	 *	Data	0x21	0xA1
	 *	Each PIC receives 8 input lines - PIC1 deals with IRQ0-7, PIC2 with IRQ8-15.
	 *	Syntax of initialisation word is crazy complex - so it works to just stick command ICW1 (0x11) in there.
	 *	This makes both of them wait for 3 more words - ICW2, its data offset, ICW3 to tell them how they're wired
	 *	(master/slave etc.), and ICW4 which gives more information about the environment.
	 *	Offset is what we add to the input line number to get the interrupt number.
	 */

	// We use the write_port function defined in kernel.asm to, uh, write to the ports. Yeah.
	// ICW1 first, 0x11 to both
	write_port(0x20, 0x11);
	write_port(0xA0, 0x11);

	// ICW2 - Set up the offset
	// Mapping beyond 0x20 since we're in x86 protected mode and Intel have made the first 32 "reserved" for cpu exceptions
	// Using 0x21 and 0xA1 since they're the data ports - 0x11 on the command line tells them to expect 3 more words on their data lines.
	write_port(0x21, 0x20);
	write_port(0xA1, 0x28);

	// ICW3 - no cascading so set both to 0
	write_port(0x21, 0x00);
	write_port(0xA1, 0x00);

	// ICW4 - environment info, here saying with the lowermost bit that we're running in 80x86 mode
	write_port(0x21, 0x01);
	write_port(0xA1, 0x01);

	// Now disable all interrupt lines - we're gonna reenable the one for keyboard later on.
	write_port(0x21, 0xff);
	write_port(0xA1, 0xff);

	// Keyboard uses IRQ1. Add to offset of 0x20 and the result is 0x21 - therefore we have to map the keyboard handler
	// address aainst interrupt 0x21 in the IDT
	// Therefore we need to populate the IDT for the interrupt 0x21 
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = 0x08;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = 0x8e;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;
		//Apparently Intel wants the higher 16 in the lower 16 bits, and vice versa because of... reasons.
	
	idt_address = (unsigned long)IDT;
	idt_prt[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16;

	load_idt(idt_ptr);	//This function is defined in kernel.asm - tells the processor where to find the IDT we just made
}

void kb_init(void) {
	// 1 is disabled, 0 is enabled - after idt_init they're all set to 1 to disable them
	write_port(0x21, 0xFD);	//0xFD is 11111101, so ONLY IRQ1 is enabled - for the keyboard.
}

void keyboard_handler_main(void) {
	// Function to handle the, uh, keyboard. Yeah.
	unsigned char status;
	char keycode;	// Need some kind of table to map this to an actual character but whatever, we'll print it anyway

	// First thing to do is signal end of interrupt - otherwise no more interrupts are allowed. This is written to the PIC's command port.
	write_port(0x20, 0x20);
	// Now we can read the keyboard input ports - 0x60 for data (keycode) and 0x64 for status/commands.
	// STATUS: if LSB is 0, then buffer is empty and there's nothing to read.
	// 			if LSB is 1, then we proceed to read the ASCII keycode from port 0x60 - read it as a char and that's keyboard input done!
	status = read_port(0x60);
	if (status & 0x01){
		keycode = read_port(0x64);
		// We can write to the screen same as before now.
		if (keycode < 0){
			return;	//Why the hell would this happen
		}
		vidptr[current_loc++] = keycode; // Just printing it for the time being
		vidptr[current_loc++] = 0x07;	// formatting = 0 for black, 7 for light grey text
	}

	return;


}




