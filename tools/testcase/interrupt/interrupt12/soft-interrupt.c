/*
 * Interrupt 12: Stack Fault Exception (#SS) by soft-interrupt
 *
 * (C) 2018.01 BiscuitOS <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>

/*
 * Trigger interrupt 12 (#SS): invoke 'int $0xC'
 *   Note! whatever interrupt is enable or disable, this routine
 *   will trigger interrupt 12.
 */
void trigger_interrupt12(void)
{
    printk("Trigger interrupt 12: Stack segment "
                              "[invoke 'int $0xC']\n");
    __asm__ ("int $0xC");
}
