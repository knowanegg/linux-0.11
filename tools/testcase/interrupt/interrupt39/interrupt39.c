/*
 * interrupt 39: Parallel Interrupt
 *
 * (C) 2017.10 BiscuitOS <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <test/debug.h>

void common_interrupt39(void)
{
    trigger_interrupt39();
}
