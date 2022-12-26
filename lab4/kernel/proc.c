
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"

#include "global.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;

	for (p =p_proc_ready+1 ; p < proc_table+NR_TASKS; p++) {
		if (get_ticks()>=p->start_time  && p->semaphore==0) {
			p_proc_ready = p;
			return;
		}
	}
	for (p = proc_table; p<=p_proc_ready ; p++) {
		if (get_ticks()>=p->start_time && p->semaphore==0) {
			p_proc_ready = p;
			return;
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}




PUBLIC void sys_delay(int time){
	p_proc_ready->start_time=get_ticks()+time/(1000/HZ)+1;
	schedule();
}

PUBLIC void sys_p(Semaphore* s){
	s->count--;
	if (s->count<0){
		p_proc_ready->semaphore=s;
		s->queue[s->tail]=p_proc_ready;
		s->tail++;
		schedule();	
	}
};

PUBLIC void sys_v(Semaphore* s){
	s->count++;
	if (s->count<=0){
		PROCESS * p=s->queue[s->head];
		p->semaphore=0;
		s->head++;
	}
};