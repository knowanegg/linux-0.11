/*
 * ramdisk.c
 *
 * Written by Theodore Ts'o, 12/2/91
 */
#include <linux/fs.h>
#include <linux/kernel.h>

#include <string.h>

char *rd_start;
int rd_length = 0;

/*
 * If the root device is the ram disk, try to load it.
 * In order to do this, the root device is originally set to the
 * floppy, and we later change it to be ram disk.
 */
void rd_load(void)
{
    struct buffer_head *bh;
    struct super_block s;
    int block = 256;         /* Start at block 256 */
    int i = 1;
    int nblocks;
    char *cp;                /* Move pointer */

    if (!rd_length)
        return;
    printk("Ram disk: %d bytes, starting at 0x%x\n", rd_length,
           (int) rd_start);
    if (MAJOR(ROOT_DEV) != 2)
        return;
    bh = breada(ROOT_DEV, block + 1, block, block + 2, -1);
    if (!bh) {
        printk("Disk error while looking for ramdisk!\n");
        return;
    }
    *((struct d_super_block *) &s) = *((struct d_super_block *)bh->b_data);
    brelse(bh);
    if (s.s_magic != SUPER_MINIX_MAGIC && 
        s.s_magic != SUPER_MINIX_MAGIC_V1)
        /* No ram disk image present, assume normal floppy boot */
        return;
    nblocks = s.s_nzones << s.s_log_zone_size;
    if (nblocks > (rd_length >> BLOCK_SIZE_BITS)) {
        printk("Ram disk image too big! (%d blocks, %d avail)\n",
               nblocks, rd_length >> BLOCK_SIZE_BITS);
        return;
    }
    printk("Loading %d bytes into ram disk... 0000k",
           nblocks << BLOCK_SIZE_BITS);
    cp = rd_start;
    while (nblocks) {
        if (nblocks > 2)
            bh = breada(ROOT_DEV, block, block + 1, block + 2, -1);
        else
            bh = bread(ROOT_DEV, block);
        if (!bh) {
            printk("I/O error on block %d, aborting load\n", block);
            return;
        }
        (void) memcpy(cp, bh->b_data, BLOCK_SIZE);
        brelse(bh);
        printk("\010\010\010\010\010%4dk",i);
        cp += BLOCK_SIZE;
        block++;
        nblocks--;
        i++;
    }
    printk("\010\010\010\010\010done \n");
    ROOT_DEV = 0x0101;
}
