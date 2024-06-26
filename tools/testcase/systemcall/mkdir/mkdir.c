/*
 * System Call: mkdir
 *
 * (C) 2018.04 BiscuitOS <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define __LIBRARY__
#include <unistd.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/segment.h>

#include <test/debug.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <const.h>

#define MAY_EXEC    1
#define MAY_WRITE   2
#define MAY_READ    4

/*
 * permission()
 *
 * is used to check for read/write/execute permission on a file.
 * I don't know if we should look at just the euid or both euid and
 * uid, but that should be easily changed.
 */
static int permission(struct m_inode *inode, int mask)
{
    int mode = inode->i_mode;

    /* special case: not even root can read/write a deleted file */
    if (inode->i_dev && !inode->i_nlinks)
        return 0;
    else if (current->euid == inode->i_uid)
        mode >>= 6;
    else if (current->egid == inode->i_gid)
        mode >>= 3;
    if (((mode & mask & 0007) == mask) || suser())
        return 1;
    return 0;
}

/*
 * Ok, we cannot use strncmp, as the name is not in our data space.
 * Thus we'll have to use match. No big problem. Match also makes
 * some sanity tests.
 *
 * NOTE! unlike strncmp, match returns 1 for success, 0 for failure.
 */
static int match(int len, const char *name, struct dir_entry *de)
{
    register int same;

    if (!de || !de->inode || len > NAME_LEN)
        return 0;
    if (len < NAME_LEN && de->name[len])
        return 0;
    __asm__ ("cld\n\t"
             "fs; repe ; cmpsb\n\t"
             "setz %%al"
             : "=a" (same)
             : "0" (0), "S" ((long)name), "D" ((long)de->name),
               "c" (len));
    return same;
}

/*
 * find_entry()
 *
 * finds an entry in the specified directory with the wanted name. IT
 * returns the cache buffer in which the entry was found, and the netry
 * itself (as a parameter - res_dir). It does NOT read the inode of the 
 * entry - you'll have to do that yourself if you want to.
 *
 * This also takes care of the few special cases due to '..' - traversal
 * over a pseudo-root and a mount point.
 */
static struct buffer_head *find_entry(struct m_inode **dir,
     const char *name, int namelen, struct dir_entry **res_dir)
{
    int entries;
    int block, i;
    struct buffer_head *bh;
    struct dir_entry *de;
    struct super_block *sb;
    
#ifdef NO_TRUNCATE
    if (namelen > NAME_LEN)
        return NULL;
#else
    if (namelen > NAME_LEN)
        namelen = NAME_LEN;
#endif
    entries = (*dir)->i_size / (sizeof(struct dir_entry));
    *res_dir = NULL;
    if (!namelen)
        return NULL;
    /* check for '..', as we might have to do some 'magic' for it */
    if (namelen == 2 && get_fs_byte(name) == '.' &&
          get_fs_byte(name + 1) == '.') {
        /* '..' in a pseudo-root results in a faked '.' (just change namelen) */
        if ((*dir) == current->root)
            namelen = 1;
        else if ((*dir)->i_num == ROOT_INO) {
            /* '..' over a mount-point results in 'dir' being exchanged
             * for the mount */
            sb = get_super((*dir)->i_dev);
            if (sb->s_imount) {
                iput(*dir);
                (*dir) = sb->s_imount;
                (*dir)->i_count++;
            }
        }
    }
    if (!(block = (*dir)->i_zone[0]))
        return 0;
    if (!(bh = bread((*dir)->i_dev, block)))
        return NULL;
    i = 0;
    de = (struct dir_entry *)bh->b_data;
    while (i < entries) {
        if ((char *)de >= BLOCK_SIZE + bh->b_data) {
            /* I don't care */;
        }
        if (match(namelen, name, de)) {
            *res_dir = de;
            return bh;
        }
        de++;
        i++;
    }
    brelse(bh);
    return NULL;
}

/*
 * get_dir()
 *
 * getdir traverses the pathname until it hits the topmost direftory.
 * It returns NULL on failure.
 */
static struct m_inode *get_dir(const char *pathname)
{
    char c;
    const char *thisname;
    struct m_inode *inode;
    struct buffer_head *bh;
    int namelen, inr, idev;
    struct dir_entry *de;

    if (!current->root || !current->root->i_count)
        panic("No root inode");
    if (!current->pwd || !current->pwd->i_count)
        panic("No pwd inode");
    if ((c = get_fs_byte(pathname)) == '/') {
        inode = current->root;
        pathname++;
    } else if (c)
        inode = current->pwd;
    else
        return NULL; /* empty name is bad */
    inode->i_count++;
    while (1) {
        thisname = pathname;
        if (!S_ISDIR(inode->i_mode) || !permission(inode, MAY_EXEC)) {
            iput(inode);
            return NULL;
        }
        for (namelen = 0; (c = get_fs_byte(pathname++)) && (c != '/');
                        namelen++)
            /* nothing */;
        if (!c)
            return inode;
        if (!(bh = find_entry(&inode, thisname, namelen, &de))) {
            iput(inode);
            return NULL;
        }
        inr = de->inode;
        idev = inode->i_dev;
        brelse(bh);
        iput(inode);
        if (!(inode = iget(idev, inr)))
            return NULL;
    }
} 

/*
 * dir_namei()
 *
 * dir_name() returns the inode of the directory of the 
 * specified name, and the name within that directory.
 */
static struct m_inode *dir_namei(const char *pathname,
       int *namelen, const char **name)
{
    char c;
    const char *basename;
    struct m_inode *dir;

    if (!(dir = get_dir(pathname)))
        return NULL;
    basename = pathname;
    while ((c = get_fs_byte(pathname++)))
        if (c == '/')
            basename = pathname;
    *namelen = pathname - basename - 1;
    *name = basename;
    return dir;
}

/*
 * add_entry()
 *
 * add a file entry to the specified directory. using the same
 * semantics as find_entry(0. It return NULL if it failed.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between  calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 */
static struct buffer_head *add_entry(struct m_inode *dir,
        const char *name, int namelen, struct dir_entry **res_dir)
{
    int block, i;
    struct buffer_head *bh;
    struct dir_entry *de;

    *res_dir = NULL;
#ifdef NO_TRUNCATE
    if (namelen > NAME_LEN)
        return NULL;
#else
    if (namelen > NAME_LEN)
        namelen = NAME_LEN;
#endif
    if (!namelen)
        return NULL;
    if (!(block = dir->i_zone[0]))
        return NULL;
    if (!(bh = bread(dir->i_dev, block)))
        return NULL;
    i = 0;
    de = (struct dir_entry *)bh->b_data;
    while (1) {
        if ((char *)de >= BLOCK_SIZE + bh->b_data) {
            /* I don't care */;
        }
        if (i * sizeof(struct dir_entry) >= dir->i_size) {
            de->inode = 0;
            dir->i_size = (i + 1) * sizeof(struct dir_entry);
            dir->i_dirt = 1;
            dir->i_ctime = CURRENT_TIME;
        }
        if (!de->inode) {
            dir->i_mtime = CURRENT_TIME;
            for (i = 0; i < NAME_LEN; i++)
                de->name[i] = (i < namelen) ? get_fs_byte(name + i) : 0;
            bh->b_dirt = 1;
            *res_dir = de;
            return bh;
        }
        de++;
        i++;
    }
    brelse(bh);
    return NULL;
}

int sys_d_mkdir(const char *pathname, int mode)
{
    const char *basename;
    int namelen;
    struct m_inode *dir, *inode;
    struct buffer_head *bh, *dir_block;
    struct dir_entry *de;

    if (!suser())
        return -EPERM;
    if (!(dir = dir_namei(pathname, &namelen, &basename)))
        return -ENOENT;
    if (!namelen) {
        iput(dir);
        return -ENOENT;
    }
    if (!permission(dir, MAY_WRITE)) {
        iput(dir);
        return -EPERM;
    }
    bh = find_entry(&dir, basename, namelen, &de);
    if (bh) {
        brelse(bh);
        iput(dir);
        return -EEXIST;
    }
    inode = new_inode(dir->i_dev);
    if (!inode) {
        iput(dir);
        return -ENOSPC;
    }
    inode->i_size = 32;
    inode->i_dirt = 1;
    inode->i_mtime = inode->i_atime = CURRENT_TIME;
    if (!(inode->i_zone[0] = new_block(inode->i_dev))) {
        iput(dir);
        inode->i_nlinks--;
        iput(inode);
        return -ENOSPC;
    }
    inode->i_dirt = 1;
    if (!(dir_block = bread(inode->i_dev, inode->i_zone[0]))) {
        iput(dir);
        free_block(inode->i_dev, inode->i_zone[0]);
        inode->i_nlinks--;
        iput(inode);
        return -ERROR;
    }
    de = (struct dir_entry *)dir_block->b_data;
    de->inode = inode->i_num;
    strcpy(de->name, ".");
    de++;
    de->inode = dir->i_num;
    strcpy(de->name, "..");
    inode->i_nlinks = 2;
    brelse(dir_block);
    inode->i_mode = I_DIRECTORY | (mode & 0777 & ~current->umask);
    inode->i_dirt = 1;
    bh = add_entry(dir, basename, namelen, &de);
    if (!bh) {
        iput(dir);
        free_block(inode->i_dev, inode->i_zone[0]);
        inode->i_nlinks = 0;
        iput(inode);
        return -ENOSPC;
    }
    de->inode = inode->i_num;
    bh->b_dirt = 1;
    dir->i_nlinks++;
    dir->i_dirt = 1;
    iput(dir);
    iput(inode);
    brelse(bh);
    return 0;
}

/* Invoke by system call: int $0x80 */
int debug_syscall_mkdir0(void)
{
    d_mkdir("/usr/bin/BiscuitOS", 0);
    return 0;
}
