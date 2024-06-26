#ifndef _TEST_INTERRUPT_H
#define _TEST_INTERRUPT_H

#ifdef CONFIG_DEBUG_INTERRUPT
extern int debug_interrupt_common(void);

#ifdef CONFIG_DEBUG_IDT
struct desc_node
{
    unsigned long a;
    unsigned long b;
};

struct gate_desc
{
    unsigned long offset;
    unsigned short sel;
    unsigned char dpl;
    unsigned char flag;
    unsigned char type;
};
extern void debug_idt_segment_desc_common(void);
#endif

#ifdef CONFIG_DEBUG_INT_USAGE
extern void interrupt_useage_common(void);
#endif

#ifdef CONFIG_DEBUG_INTERRUPT0
extern void common_interrupt0(void);

#if defined CONFIG_DEBUG_DIVIDE_ZERO || defined CONFIG_DEBUG_OVERFLOW_BIT || \
    defined CONFIG_DEBUG_SOFT_INT0
extern void trigger_interrupt0(void);
#endif

#endif // INTERRUPT 0

#ifdef CONFIG_DEBUG_INTERRUPT1
extern void common_interrupt1(void);

#if defined CONFIG_DEBUG_SOFT_INT1 || defined CONFIG_DEBUG_EFLAGS_TF
extern void trigger_interrupt1(void);
#endif

#endif // INTERRUPT 1

#ifdef CONFIG_DEBUG_INTERRUPT2
extern void common_interrupt2(void);

#ifdef CONFIG_DEBUG_SOFT_INT2
extern void trigger_interrupt2(void);
#endif

#endif // INTERRUPT 2

#ifdef CONFIG_DEBUG_INTERRUPT3
extern void common_interrupt3(void);

#if defined CONFIG_DEBUG_SOFT_INT3 || defined CONFIG_DEBUG_INT3_TF
extern void trigger_interrupt3(void);
#endif

#endif // INTERRUPT 3

#ifdef CONFIG_DEBUG_INTERRUPT4
extern void common_interrupt4(void);

#if defined CONFIG_DEBUG_SOFT_INT4 || defined CONFIG_DEBUG_INT4_INTO
extern void trigger_interrupt4(void);
#endif

#endif // INTERRUPT 4

#ifdef CONFIG_DEBUG_INTERRUPT5
extern void common_interrupt5(void);

#if defined CONFIG_DEBUG_SOFT_INT5 || defined CONFIG_DEBUG_INT5_BOUND || \
    defined CONFIG_DEBUG_INT5_BNDCL || defined CONFIG_DEBUG_INT5_BNDCU
extern void trigger_interrupt5(void);
#endif

#endif // INTERRUPT 5

#ifdef CONFIG_DEBUG_INTERRUPT6
extern void common_interrupt6(void);

#if defined CONFIG_DEBUG_SOFT_INT6 || defined CONFIG_DEBUG_INT6_BOUND
extern void trigger_interrupt6(void);
#endif

#endif // INTERRUPT 6

#ifdef CONFIG_DEBUG_INTERRUPT7
extern void common_interrupt7(void);

#if defined CONFIG_DEBUG_SOFT_INT7
extern void trigger_interrupt7(void);
#endif

#endif // INTERRUPT 7

#ifdef CONFIG_DEBUG_INTERRUPT8
extern void common_interrupt8(void);

#if defined CONFIG_DEBUG_SOFT_INT8
extern void trigger_interrupt8(void);
#endif

#endif // INTERRUPT 8

#ifdef CONFIG_DEBUG_INTERRUPT9
extern void common_interrupt9(void);

#if defined CONFIG_DEBUG_SOFT_INT9
extern void trigger_interrupt9(void);
#endif

#endif // INTERRUPT 9

#ifdef CONFIG_DEBUG_INTERRUPT10
extern void common_interrupt10(void);

#if defined CONFIG_DEBUG_SOFT_INT10
extern void trigger_interrupt10(void);
#endif

#endif // INTERRUPT 10

#ifdef CONFIG_DEBUG_INTERRUPT11
extern void common_interrupt11(void);

#if defined CONFIG_DEBUG_SOFT_INT11 || defined CONFIG_DEBUG_INT11_LLDT || \
  defined CONFIG_DEBUG_INT11_LTR  || defined CONFIG_DEBUG_INT11_MOV_DS || \
  defined CONFIG_DEBUG_INT11_MOV_ES || defined CONFIG_DEBUG_INT11_MOV_FS || \
  defined CONFIG_DEBUG_INT11_MOV_GS || defined CONFIG_DEBUG_INT11_POP_DS || \
  defined CONFIG_DEBUG_INT11_POP_ES || defined CONFIG_DEBUG_INT11_POP_FS || \
  defined CONFIG_DEBUG_INT11_POP_GS
extern void trigger_interrupt11(void);
#endif

#endif // INTERRUPT 11

#ifdef CONFIG_DEBUG_INTERRUPT12
extern void common_interrupt12(void);

#if defined CONFIG_DEBUG_SOFT_INT12
extern void trigger_interrupt12(void);
#endif

#endif // INTERRUPT12

#ifdef CONFIG_DEBUG_INTERRUPT13
extern void common_interrupt13(void);

#if defined CONFIG_DEBUG_SOFT_INT13
extern void trigger_interrupt13(void);
#endif

#endif // INTERRUPT13

#ifdef CONFIG_DEBUG_INTERRUPT14
extern void common_interrupt14(void);

#if defined CONFIG_DEBUG_SOFT_INT14
extern void trigger_interrupt14(void);
#endif

#endif // INTERRUPT14

#ifdef CONFIG_DEBUG_INTERRUPT16
extern void common_interrupt16(void);

#if defined CONFIG_DEBUG_SOFT_INT16
extern void trigger_interrupt16(void);
#endif

#endif // INTERRUPT16

#ifdef CONFIG_DEBUG_INTERRUPT32
extern void common_interrupt32(void);

#if defined CONFIG_DEBUG_SOFT_INT32
extern void trigger_interrupt32(void);
#endif

#endif // INTERRUPT32

#ifdef CONFIG_DEBUG_INTERRUPT39
extern void common_interrupt39(void);

#if defined CONFIG_DEBUG_SOFT_INT39
extern void trigger_interrupt39(void);
#endif

#endif // INTERRUPT39

#ifdef CONFIG_DEBUG_INTERRUPT45
extern void common_interrupt45(void);

#if defined CONFIG_DEBUG_SOFT_INT45
extern void trigger_interrupt45(void);
#endif

#endif // INTERRUPT45

#ifdef CONFIG_DEBUG_INTERRUPT46
extern void common_interrupt46(void);

#if defined CONFIG_DEBUG_SOFT_INT46
extern void trigger_interrupt46(void);
#endif

#endif // INTERRUPT46

#ifdef CONFIG_DEBUG_INTERRUPT128
extern void common_interrupt128(void);

#if defined CONFIG_DEBUG_SOFT_INT128
extern void trigger_interrupt128();
#endif

#endif // INTERRUPT128

#endif // end of CONFIG_TESTCASE_INTERRUPT
#endif
