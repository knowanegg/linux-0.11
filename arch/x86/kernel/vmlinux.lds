/*
 * vmlinux.ld
 *
 * Linker script for the i386 vmlinux.
 */
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(startup_32)

SECTIONS
{
	. = 0x00;
	.text		: { *(.text) }
	.data		: { *(.data) }
	.bss		: { *(.bss) }
	PROVIDE (end = .);
}
