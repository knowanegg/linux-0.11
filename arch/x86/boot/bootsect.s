# 从bios跳转到这里，也就是0x7c00开始执行
# 这里开始就是0x7c00不需要在编译设置偏移
# 是因为BIOS已经把硬盘0扇区也就是这个程序
# 挪到了0x7c00，然后在BIOS最后一条指令
# jmp到了0x7c00

# 编译完后，用objdump看vmlinux，开头是head.s
# 那么这个bootsect和setup在哪呢？
# 应该是在硬盘1、2扇区，而vmlinux被写入到了
# 从第三个扇区开始
# 我们想找到真的编译成果，就去找硬盘文件
# 看makefile，实际编译出来的合并到了bootloader
# objdump -D -b binary -m i386:x86-64 ./arch/x86/boot/bootloader

# 第一步，先把自己移动到0x90000，为后面
# 腾出空间

# 首先通过ax设置es到0x9000
# 注意这里还没有进入保护模式，所以现在还是
# 用ax，16位，也不用movl而是mov
.code16 # 设置编译成16位指令
	.global _start
# 为什么一定要_start?
# _start相当于c语言的main！必须存在的入口
# 链接器会找_start,名称不能变

_start:
    mov $0x9000, %ax
    mov %ax, %es
# 现在设置好了es目标段，设置当前段ds
    mov $0x07c0, %ax
    mov %ax, %ds
# 循环移动本身,大小为512也就是0x200大小
# 用movsw可以一次移动两个byte，除以二=0x100=256
    mov $256, %cx
# 清理si和di
    xor %si, %di
# 循环拷贝
    rep movsw

# 拷贝完后，跳转到新地址执行
# 这里有的是jmpf，有的是ljmp，看不同编译器
# ljmp seg, ip
    ljmp $0x9000, $new_start

new_start:
    mov %ax, %bx
    jmp new_start

.org 510
mbr_id:
	.word 0xAA55
