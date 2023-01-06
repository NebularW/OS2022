/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"

#include "global.h"

Semaphore  rmutex;			//读操作
Semaphore  wmutex;			//写操作
Semaphore  mutex;			//修改readerCount
int readerCount;

int COND_B;
int COND_C;
int COND_D;
int COND_E;
int COND_F;

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 15;
	proc_table[1].ticks = proc_table[1].priority =  5;
	proc_table[2].ticks = proc_table[2].priority =  3;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */
        

	readerCount=0;
	rmutex.count=3;
	rmutex.head=0;
	rmutex.tail=0;
	wmutex.count=1;
	wmutex.head=0;
	wmutex.tail=0;
	mutex.count=1;
	mutex.head=0;
	mutex.tail=0;
	COND_B = COND_C = COND_D = COND_E = COND_F = 0;
 	clearScreen();

	restart();

	while(1){}
}

void clearScreen(){
	u8* base=(u8*) 0xB8000;
	for (int i=0;i<0x8000;i=i+2){
		base[i]=' ';
		
	}
	disp_pos=0;
	lines=1;
}

void A()
{
	while(1){
		if (lines < 21) {
			char* num = "xx";
			num[0] = (lines / 10) + '0';
			num[1] = (lines % 10) + '0';
			s_disp_str(num);
			s_disp_str(" ");
			if (COND_B == 1) {
				disp_color_str("O",0x0A);
			} else if (COND_B == 0) {
				disp_color_str("X",0x0C);
			} else {
				disp_color_str("Z",0x01);
			}
			s_disp_str(" ");
			if (COND_C == 1) {
				disp_color_str("O",0x0A);
			} else if (COND_C == 0) {
				disp_color_str("X",0x0C);
			} else {
				disp_color_str("Z",0x01);
			}
			s_disp_str(" ");
			if (COND_D == 1) {
				disp_color_str("O",0x0A);
			} else if (COND_D == 0) {
				disp_color_str("X",0x0C);
			} else {
				disp_color_str("Z",0x01);
			}
			s_disp_str(" ");
			if (COND_E == 1) {
				disp_color_str("O",0x0A);
			} else if (COND_E == 0) {
				disp_color_str("X",0x0C);
			} else {
				disp_color_str("Z",0x01);
			}
			s_disp_str(" ");
			if (COND_F == 1) {
				disp_color_str("O",0x0A);
			} else if (COND_F == 0) {
				disp_color_str("X",0x0C);
			} else {
				disp_color_str("Z",0x01);
			}
		
			s_disp_str("\n");
			lines++;
		}
		
		milli_delay(3000);
	}
}


void B()
{
	while (1) {
		p(&rmutex);
		p(&mutex);
		readerCount++;
		if (readerCount==1) p(&wmutex);
		v(&mutex);
		
		COND_B = 1;
		milli_delay(2*3000);
		COND_B = 2;
		
		p(&mutex);
		readerCount--;
		if (readerCount==0) v(&wmutex);
		v(&mutex);
		v(&rmutex);
		milli_delay(3000); 
		COND_B = 0;
	}
}

void C()
{
	while (1) {
		p(&rmutex);
		p(&mutex);
		readerCount++;
		if (readerCount==1) p(&wmutex);
		v(&mutex);
		
		COND_C = 1;
		milli_delay(3*3000);
		COND_C = 2;
		
		p(&mutex);
		readerCount--;
		if (readerCount==0) v(&wmutex);
		v(&mutex);
		v(&rmutex);
		milli_delay(3000); 
		COND_C = 0;
	}
}

void D()
{
	while (1) {
		p(&rmutex);
		p(&mutex);
		readerCount++;
		if (readerCount==1) p(&wmutex);
		v(&mutex);
		
		COND_D = 1;
		milli_delay(3*3000);
		COND_D = 2;
		
		p(&mutex);
		readerCount--;
		if (readerCount==0) v(&wmutex);
		v(&mutex);
		v(&rmutex);
		milli_delay(3000); 
		COND_D = 0;
	}
}

void E()
{
	while (1) {
		p(&wmutex);
		COND_E = 1;
		milli_delay(3*3000);
		COND_E = 2;
		v(&wmutex);
		milli_delay(3000); 
		COND_E = 0;
	}
}

void F()
{
	while(1){
		p(&wmutex);
		COND_F = 1;
		milli_delay(4*3000);
		COND_F = 2;
		v(&wmutex);
		milli_delay(3000); 
		COND_F = 0;
	}
}