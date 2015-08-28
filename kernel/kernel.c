/* kernel.c
 *
 */

void kmain(void){	//kmain was called in kernel.asm; returning from it halts the processor
	// At the moment the aim is to clear the screen and write some text to it
	const char *str = "Hello, world!";	// String to write to screen
	char *vidptr = (char*)0xb8000;		/* Start of relevant bit of video memory
										 * In REAL mode, video memory of VGA is mapped to PC memory between segments 0xA0000 and 0xBFFFF
										 * This breaks down as:
										 * 0xA0000 for EGA/VGA graphics modes (64KB)
										 * 0xB0000 for monochrome text mode (32KB)
										 * 0xB8000 for colour text mode and CGA compatible graphics modes.
										 */

	// Loop to clear the screen - 25 lines of 80 columns each. Each element takes 2 bytes, one for ASCII code and one for formatting
	unsigned int i = 0;
	unsigned int j = 0;
	while(j < (80 * 25 * 2)){
		vidptr[j] = ' ';	// Replace whatever it is with a blank character
		vidptr[j+1] = 0x07;	// Formatting/attribute byte - light grey on black, refer to chart for complete list
		j = j + 2;	// Jump to next character's pair of bytes
	}

	j = 0;	// Now i and j are back to 0

	// Now write the string over the top of the blank screen
	while(str[j] != '\0') {	// Until the eol character...
		vidptr[i] = str[j];		// Write ASCII byte
		vidptr[i+1] = 0x07;		// 0x07 in attribute byte means light grey (the 7) on black background (0).
		j++;					// Step to next character to be written
		i = i + 2;				// Step to next space in screen memory to be written to.
	}
	return;
}





	
