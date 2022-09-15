OUTPUT_ARCH(arm)

MEMORY
{
	RAMRO (rx) : ORIGIN = 0x0, LENGTH = 0x0000C000
}

SECTIONS
{
	.text : ALIGN(4) {
		build/crt0.o(.init)
		*(.text)
	}

	/DISCARD/ : {
		*(.dyn*)
		*(.hash)
		*(.rel*)
		*(.eh_frame)
	}   

	.rodata ALIGN(0x4) :
	{
		*(.rodata*)
		*(.got*)
		. = ALIGN(4);
	}
}

