
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				  console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

/* CONSOLE */
typedef struct s_console
{
	unsigned int current_start_addr; /* 当前显示到了什么位置	  */
	unsigned int original_addr;		 /* 当前控制台对应显存位置 */
	unsigned int v_mem_limit;		 /* 当前控制台占的显存大小 */
	unsigned int cursor;			 /* 当前光标位置 */
	unsigned int start_cursor;		 /* 原来光标位置 */
} CONSOLE;

#define SCR_UP 1  /* scroll forward */
#define SCR_DN -1 /* scroll backward */

#define SCREEN_SIZE (80 * 25)
#define SCREEN_WIDTH 80

#define DEFAULT_CHAR_COLOR 0x07 /* 0000 0111 黑底白字 */
#define RED_CHAR_COLOR 0x04		/* 0000 0100 黑底红字 */
#define GREEN_CHAR_COLOR 0x02	/* 0000 0010 黑底绿字 */
#define BLUE_CHAR_COLOR 0x01	/* 0000 0001 黑底蓝字 */
#define TAB_WIDTH 4				/*TAB的长度是4*/

#endif /* _ORANGES_CONSOLE_H_ */