
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				  console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;


	set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   exit_esc
*======================================================================*/
void clearKeyWords(CONSOLE *p_con)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	int len = p_con->cursor - p_con->start_cursor;
	for (int i = 0; i < len; i++)
	{
		*(p_vmem - 2 - 2 * i) = ' ';
		*(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
	}
}

void clearColor(CONSOLE *p_con)
{
	for (int i = 0; i < p_con->start_cursor * 2; i += 2)
	{
		*(u8 *)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
	}
}

PUBLIC void exit_esc(CONSOLE *p_con)
{

	//把关键字清除
	clearKeyWords(p_con);
	//退出时，红色的都变回白色
	clearColor(p_con);
	//光标恢复到进入esc模式之前，ptr也复原
	p_con->cursor = p_con->start_cursor;

	flush(p_con); //设置好要刷新才能显示
}

/*======================================================================*
			   search_key_word
*======================================================================*/
PUBLIC void search(CONSOLE *p_con)
{
	int keyLen = p_con->cursor * 2 - p_con->start_cursor * 2;
	for (int i = 0; i < p_con->start_cursor * 2; i += 2)
	{
		int begin = i;
		int end = i;
		int found = 1;

		for (int j = 0; j < keyLen; j += 2)
		{
			// matching
			char src = *((u8 *)(V_MEM_BASE + end));
			int place = j + p_con->start_cursor * 2;
			char tgt = *((u8 *)(V_MEM_BASE + place));

			if (src == tgt)
			{
				end += 2;
			}
			else
			{
				found = 0;
				break;
			}
		}

		if (found == 1)
		{
			//如果找到，就标红
			for (int m = begin; m < end; m += 2)
			{
				if(*(u8 *)(V_MEM_BASE + m) == 0 || *(u8 *)(V_MEM_BASE + m) == ' '){
					*(u8 *)(V_MEM_BASE + m + 1) = RED_BACKGROUND;
				}else{
					*(u8 *)(V_MEM_BASE + m + 1) = RED_CHAR_COLOR; //只需要设置颜色，所以更改每个字母的第二个字节即可
				}
				

			}
		}
	}
	
	flush(p_con);
}

/*======================================================================*
			   out_char
 *======================================================================*/

PUBLIC void out_char(CONSOLE *p_con, char ch)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

	switch (ch)
	{
	case '\n':
		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - SCREEN_WIDTH)
		{
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
															SCREEN_WIDTH +
														1);
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			if (*(p_vmem - 2) == 0)
			{
				p_con->cursor -= 4;
			}
			else
			{
				p_con->cursor--;
				*(p_vmem - 2) = ' ';
				*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
			}
		}
		break;
	case '\t':
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			for (int i = 0; i < 4; i++)
			{
				*p_vmem++ = 0;
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}
			p_con->cursor += TAB_WIDTH;
		}
		break;
	default:
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			*p_vmem++ = ch;
			*p_vmem++ = DEFAULT_CHAR_COLOR;
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
						   flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
				set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
			p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}
