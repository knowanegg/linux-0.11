Interrupt 12 -- Stack Fault Exception (#SS)
----------------------------------------------------

### Description

  Indicates that one of the following stack related conditions was detected:
 
  * A limit violation is detected during an operation that refers to
    the SS register. Operations that can cause a limit violation include
    stack-oriented instructions such as POP, PUSH, CALL, RET, ENTER, and
    LEVE, as well as other memory references which implicitly or explicitly
    use the SS register (for example, `MOV AX, [BP+6]` or `MOV AX, SS:[EAX+6]`
    ). The `ENTER` instruction generates this exception when there is not
    enough stack space for allocating local variables.

  * A not-present stack segment is detected when attempting to load the 
    SS register. This violation can occur during the exception of a stack
    switch, a CALL instruction to a different privilegel level, a return to
    a different privilege level, an LSS instruction, or a MOV or POP 
    instruction to the SS register.

  Recovery from this fault is possible by either extending the limit of the
  stack segment (in the case of a limit violation) or loading the missing
  stack segment into memory (in the case of a not-present violation).

  In the case of a canonical violation that was caused intentionally by
  software, recovery is possible by loading the correct canonical value
  into RSP. Otherwise, a canonical violation of the address in RSP likely 
  reflects some register corruption in the software.

### Exception Error Code

  If the exception is caused by a not-present stack segment or by overflow
  of the new stack during an inter-privilege level call, the error code
  contains a segment selector for the segment that caused the exception.
  Here, the exception handler can test the present flag in the segment
  descriptor pointed to by the segment selector to determine the cause
  of the exception. For a normal limit violation (on a stack segment already
  in use) the error code is set to 0.

### Saved Instruction Pointer

  If saved contents of CS and EIP register generally point to the instruction
  that generated the exception. However, when the exception result from 
  attempting to load a not-present stack segment during a task swithc, the
  CS and EIP register point to the first instruction of the new stack.

### Program State Change

  A program-state change does not generally accompany a stack-fault exception,
  because the instruction that generated. Here, the instruction can be 
  restarted after the exception handler has corrected the stack fault 
  condition.

  If a stack fault occurs during a task switch, it occurs after the commit-to-
  new-stack. Here, the processor loads all the state information from the 
  new TSS (without performing any additional limit, present, or type checks)
  before it generates the exception. The stack fault handler should thus not
  rely on being able to use the segment selectors found in the CS, SS, DS, FS
  and GS register without causing another exception. The exception handler 
  check all segment registers before trying to resume the new stack. 
  Otherwise, general protection fault may result later under conditions that
  are more difficult to diagnose.
### File list

  * interrupt12.c

    Common entry for triggering interrupt 10 (#TS) to be invoked by top
    interface.
 
  * soft-interrupt.c

    Describe how to trigger interrupt 10 (#TS) through soft-interrupt routine.

  * README.md

    Describe the basic information for interrupt 10 (#TS).

### Usage on BiscuitOS

  The kernel of BiscuitOS supports debug Interrupt10 online. Developer utilize
  `qemu` and `Kbuild` to debug #TS that contains specific triggered condition.
  please follow these action:

  1. Specify triggered condition with `Kbuild`.

     On top of kernel source tree, utilize `make menuconfig` to configure
     specific triggered condition for #TS. as follow:

     ```
       cd */BiscuitOS/kernel
       make menuconfig
     ```

     Then, developer will obtain figure as follow, please choose `Kernel 
     hacking`.

     ![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/BiscuitOS/BiscuitOS_common_Kbuild.png)

     The next figure, set `Debug/Running kernel` as `Y` and select `TestCase
     configuration`

     ![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/BiscuitOS/kernel_hacking/kernel_hacking.png)

     Now, set `Testcase for kernel function` as `Y` and select `Interrupt 
     Machanism on X86 Arichitecture`

     ![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/BiscuitOS/kernel_hacking/testcase/TestCase.png)

     Then, set `Debug Interrupt Machanism on X86 Architecture` and go on
     selecting `Interrupt 10 - Invalid TSS Exception (#TS)`.

     ![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/BiscuitOS/kernel_hacking/testcase/interrupt/INT_INT5_TOP.png)

     Finally, set `Interrupt 10 - Invalid TSS Exception (#TS)` as `Y`
     and choose a specific triggered condition to trigger #TS.

     ![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/BiscuitOS/kernel_hacking/testcase/interrupt/INT_INT5_MENU.png)

  2. Running and Debugging #TS

     The system default enable running BiscuitOS on `qemu`, so develper can
     running and debugging #9 as follow:

     ```
       cd */BiscuitOS/kernel
       make
       make start
     ```

     The Running-Figure as follow:

     ![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/BiscuitOS/kernel_hacking/testcase/interrupt/INT_INT5_RUN.png)

  3. Demo Code

     Developer can refer and review demo code on kernel procedure to debug or 
     prevent #TS. The demo code will indicate the triggered condition for #TS
     on kernel, so developer should review demo code details and prevent 
     #TS on your procedure. For example:

     ```
       /*
        * Trigger interrupt 10: invoke 'int $0xA'
        *   Note! whatever interrupt is enable or disable, this routine
        *   will trigger interrupt 10.
        */
       void trigger_interrupt10(void)
       {
           printk("Trigger interrupt 10: invalid TSS segment "
                               "[invoke 'int $0xA']\n");
           __asm__ ("int $0xA");
       }
     ```
