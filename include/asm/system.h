#ifndef _ASM_SYSTEM_H_
#define _ASM_SYSTEM_H_

#define move_to_user_mode() \
        __asm__("movl %%esp, %%eax\n\t"   \
                "pushl $0x17\n\t"   \
                "pushl %%eax\n\t"   \
                "pushfl\n\t"        \
                "pushl $0x0f\n\t"   \
                "pushl $1f\n\t"     \
                "iret\n"            \
                "1:\t"              \
                "movl $0x17, %%eax\n\t"  \
                "movw %%ax, %%ds\n\t"    \
                "movw %%ax, %%es\n\t"    \
                "movw %%ax, %%fs\n\t"    \
                "movw %%ax, %%gs\n\t"    \
                :::"ax")

#define sti()  __asm__("sti"::)
#define cli()  __asm__("cli"::)
#define nop()  __asm__("nop"::)

#define irq_disable	sti
#define irq_enable	cli

#define iret() __asm__("iret"::)

// dpl和type在这里是类型，是位标志,0x8000是P位为1,trap是15也就是f
// gate_addr是门描述符存放的地址
#define _set_gate(gate_addr, type, dpl, addr)  \
        __asm__ ("movw %%dx, %%ax\n\t"             \
                 "movw %0, %%dx\n\t"               \
                 "movl %%eax, %1\n\t"              \
                 "movl %%edx, %2"              \
                 :                                 \
                 : "i" ((short) (0x8000 + (dpl << 13) + (type << 8))), \
                 "o" (*((char *) (gate_addr))),    \
                 "o" (*(4+(char *) (gate_addr))),  \
                 "d" ((char *) (addr)), "a" (0x00080000))
// idt定义在include/linux/head.h 0x54b8
// divide_error在asm.h，是一个偏移
/* 0x00010f5e <+33>:    mov    eax,0x108c1
   0x00010f64 <+39>:    mov    edx,eax
   0x00010f66 <+41>:    mov    eax,0x80000
   0x00010f6b <+46>:    mov    ax,dx
   0x00010f6e <+49>:    mov    dx,0x8f00 #15=f
   0x00010f72 <+53>:    mov    DWORD PTR [esi],eax
   0x00010f74 <+55>:    mov    DWORD PTR [ebx],edx
   # 这里为什么会有一个ax和dx？因为IDT表如下：
   31                16 15   14  13  12  11  8 7      5 4    0
        +-----------------+----+----+--+--+----+----+----+----+
        | Offset 31..16   | P  | DPL| 0| S|Type| 0 0 0 0 0| IST|
        +-----------------+----+----+--+--+----+----+----+----+
        | Segment Selector|          Offset 15..0            |
        +-----------------+---------------------------------+
    这里最直观来看，我们先将offset的值放到eax，但上图中，offset是分开的
    所以要再交换eax和edx的低位，就变成了高位offset和描述在一起，
    段选择符和低位offset在一起
*/
#define set_trap_gate(n, addr)  \
	_set_gate(&idt[n], 15, 0, addr)

#define set_intr_gate(n, addr)  \
	_set_gate(&idt[n], 14, 0, addr)

#define set_system_gate(n, addr) \
	_set_gate(&idt[n], 15, 3, addr)

#define _set_tssldt_desc(n, addr, type)  \
        __asm__("movw $104, %1\n\t"          \
                "movw %%ax, %2\n\t"          \
                "rorl $16, %%eax\n\t"        \
                "movb %%al, %3\n\t"          \
                "movb $" type ", %4\n\t"     \
                "movb $0x00, %5\n\t"         \
                "movb %%ah, %6\n\t"          \
                "rorl $16, %%eax"            \
                :: "a" (addr), "m" (*(n)), "m" (*(n + 2)), "m" (*(n + 4)), \
                "m" (*(n + 5)), "m" (*(n + 6)), "m" (*(n + 7))  \
)

#define set_tss_desc(n, addr) \
    _set_tssldt_desc(((char *) (n)), ((int)(addr)), "0x89")

#define set_ldt_desc(n, addr) \
    _set_tssldt_desc(((char *) (n)), ((int)(addr)), "0x82")

#endif
