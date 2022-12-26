
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

Semaphore rmutex;
Semaphore wmutex;
Semaphore z;
int allWriter;

char state[5]; // 记录各进程状态: O:正在 X：等待 Z：休息
int t = 1;	   // 进程休息时间片长
char *to_char(int lines);

/*======================================================================*
							kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 15;
	proc_table[1].ticks = proc_table[1].priority = 5;
	proc_table[2].ticks = proc_table[2].priority = 3;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */

	readerCount = 0;
	allWriter = 0;
	rmutex.count = 3; // 同时读的数量n
	rmutex.head = 0;
	rmutex.tail = 0;
	wmutex.count = 1;
	wmutex.head = 0;
	wmutex.tail = 0;
	z.count = 1;
	z.head = 0;
	z.tail = 0;
	clearScreen();

	restart();

	while (1)
	{
	}
}

void clearScreen()
{
	u8 *base = (u8 *)0xB8000;
	for (int i = 0; i < 0x8000; i = i + 2)
	{
		base[i] = ' ';
	}
	disp_pos = 0;
	lines = 0;
}

/*======================================================================*
							   TestA
 *======================================================================*/
void A()
{
	s_disp_str("NO B C D E F\n");
	lines++;
	for (int i = 0; i < 5; i++)
		state[i] = 'X';
	while (1)
	{
		if (lines > 20)
			continue;
		s_disp_str(to_char(lines));
		s_disp_str(" ");
		for (int i = 0; i < 5; i++)
		{
			switch (state[i])
			{
			case 'O':
				disp_color_str("O ", 0x0A);
				break;
			case 'X':
				disp_color_str("X ", 0x0C);
				break;
			case 'Z':
				disp_color_str("Z ", 0x0B);
				break;
			default:
				break;
			}
		}
		s_disp_str("\n");
		lines++;
		milli_delay(3000);
	}
}

void B()
{
	while (1)
	{
		p(&rmutex);
		p(&z);
		v(&z);
		if (readerCount == 0)
			p(&wmutex);
		readerCount++;
		state[0] = 'O';
		milli_delay(2 * 3000); // B消耗2个时间片
		readerCount--;
		if (readerCount == 0)
			v(&wmutex);
		v(&rmutex);
		state[0] = 'Z';
		milli_delay(t * 3000);
		state[0] = 'X';
	}
}

void C()
{
	while (1)
	{
		p(&rmutex);
		p(&z);
		v(&z);
		if (readerCount == 0)
			p(&wmutex);
		readerCount++;
		state[1] = 'O';
		milli_delay(3 * 3000); // C消耗3个时间片
		readerCount--;
		if (readerCount == 0)
			v(&wmutex);
		v(&rmutex);
		state[1] = 'Z';
		milli_delay(t * 3000);
		state[1] = 'X';
	}
}

void D()
{
	while (1)
	{
		p(&rmutex);
		p(&z);
		v(&z);
		if (readerCount == 0)
			p(&wmutex);
		readerCount++;
		state[2] = 'O';
		milli_delay(3 * 3000); // D消耗3个时间片
		readerCount--;
		if (readerCount == 0)
			v(&wmutex);
		v(&rmutex);
		state[2] = 'Z';
		milli_delay(t * 3000);
		state[2] = 'X';
	}
}

void E()
{
	while (1)
	{
		allWriter++;
		if (allWriter == 1)
		{
			p(&z);
		}
		p(&wmutex);
		state[3] = 'O';
		milli_delay(3 * 3000); // E消耗3个时间片
		v(&wmutex);
		allWriter--;
		if (allWriter == 0)
		{
			v(&z);
		}
		state[3] = 'Z';
		milli_delay(t*3000);
		state[3] = 'X';
	}
}

void F()
{
	while (1)
	{
		allWriter++;
		if (allWriter == 1)
		{
			p(&z);
		}
		p(&wmutex);
		state[4] = 'O';
		milli_delay(4 * 3000); // F消耗4个时间片
		v(&wmutex);
		allWriter--;
		if (allWriter == 0)
		{
			v(&z);
		}
		state[4] = 'Z';
		milli_delay(t*3000);
		state[4] = 'X';
	}
}

char *to_char(int lines)
{
	char *res;
	if (lines < 10)
	{
		res[0] = lines + '0';
		res[1] = ' ';
		res[2] = '\0';
	}
	else
	{
		res[1] = lines % 10 + '0';
		res[0] = lines / 10 + '0';
		res[3] = '\0';
	}
	return res;
}