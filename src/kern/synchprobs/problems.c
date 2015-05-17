/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>
#include <wchan.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.
struct lock *Control_lock;// one central lock
int male_count=0;
int female_count=0;
int matchmaker_count=0;
void whalemating_init() {

  Control_lock=lock_create("Control_lock");
 
  return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {
  return;
}

void
male(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  male_start();
	lock_acquire(Control_lock);
	male_count++;
	while(female_count==0 || matchmaker_count==0)
	{
		lock_release(Control_lock);
                 wchan_lock(Control_lock->lk_wchan);
		 wchan_sleep(Control_lock->lk_wchan);
                 lock_acquire(Control_lock);
	}
	female_count--;
	male_count--;
	matchmaker_count--;
	lock_release(Control_lock);
	// Implement this function 
  male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  female_start();
	lock_acquire(Control_lock);
	female_count++;
	while(male_count==0 || matchmaker_count==0)
	{
		lock_release(Control_lock);
                 wchan_lock(Control_lock->lk_wchan);
		 wchan_sleep(Control_lock->lk_wchan);
                 lock_acquire(Control_lock);
	}
	female_count--;
	male_count--;
	matchmaker_count--;
	lock_release(Control_lock);
	// Implement this function 
  female_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  matchmaker_start();
	lock_acquire(Control_lock);
	matchmaker_count++;
	while(female_count==0 || male_count==0)
	{
		lock_release(Control_lock);
                 wchan_lock(Control_lock->lk_wchan);
		 wchan_sleep(Control_lock->lk_wchan);
                 lock_acquire(Control_lock);
	}
	female_count--;
	male_count--;
	matchmaker_count--;
	lock_release(Control_lock);
	// Implement this function 
  matchmaker_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.
struct lock *C_lock;
struct lock *lock_0;
struct lock *lock_1;
struct lock *lock_2;
struct lock *lock_3;

void stoplight_init() {

  C_lock=lock_create("C_lock"); // c_lock stands for 1 central lock, we might not even need this in the end.
  lock_0=lock_create("lock_0");
  lock_1=lock_create("lock_1");
  lock_2=lock_create("lock_2");
  lock_3=lock_create("lock_3");
  return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {
  return;
}

void
gostraight(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  (void)direction;
  lock_acquire(C_lock);
  if(direction==0)
  {
	lock_acquire(lock_0);
	lock_acquire(lock_3);
	inQuadrant(0);
	inQuadrant(3);
	leaveIntersection();
	lock_release(lock_0);
	lock_release(lock_3);
   }
  if(direction==1)
  {
  	lock_acquire(lock_1);
	lock_acquire(lock_0);
	inQuadrant(1);
	inQuadrant(0);
	leaveIntersection();
	lock_release(lock_1);
	lock_release(lock_0);
   }
  if(direction==2)
  {
	lock_acquire(lock_2);
	lock_acquire(lock_1);
	inQuadrant(2);
	inQuadrant(1);
	leaveIntersection();
	lock_release(lock_2);
	lock_release(lock_1);
   }
  if(direction==3)
  {
	lock_acquire(lock_3);
	lock_acquire(lock_2);
	inQuadrant(3);
	inQuadrant(2);
	leaveIntersection();
	lock_release(lock_3);
	lock_release(lock_2);
	
   }
lock_release(C_lock);
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}


void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  (void)direction;
   lock_acquire(C_lock);
  if(direction==0)
  {
	lock_acquire(lock_0);
	inQuadrant(0);
	leaveIntersection();
	lock_release(lock_0);
   }
  if(direction==1)
  {
  	lock_acquire(lock_1);
	inQuadrant(1);
	leaveIntersection();
	lock_release(lock_1);
   }
  if(direction==2)
  {
	lock_acquire(lock_2);
	inQuadrant(2);
	leaveIntersection();
	lock_release(lock_2);
   }
  if(direction==3)
  {
	lock_acquire(lock_3);
	inQuadrant(3);
	leaveIntersection();
	lock_release(lock_3);
	
   }
lock_release(C_lock);
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  (void)direction;
  lock_acquire(C_lock);
  if(direction==0)
  {
	lock_acquire(lock_0);
	lock_acquire(lock_3);
	lock_acquire(lock_2);
	inQuadrant(0);
	inQuadrant(3);
	inQuadrant(2);
	leaveIntersection();
	lock_release(lock_0);
	lock_release(lock_3);
	lock_release(lock_2);
   }
  if(direction==1)
  {
  	lock_acquire(lock_1);
	lock_acquire(lock_0);
	lock_acquire(lock_3);
	inQuadrant(1);
	inQuadrant(0);
	inQuadrant(3);
	leaveIntersection();
	lock_release(lock_1);
	lock_release(lock_0);
	lock_release(lock_3);
   }
  if(direction==2)
  {
	lock_acquire(lock_2);
	lock_acquire(lock_1);
	lock_acquire(lock_0);
	inQuadrant(2);
	inQuadrant(1);
	inQuadrant(0);
	leaveIntersection();
	lock_release(lock_2);
	lock_release(lock_1);
	lock_release(lock_0);
   }
  if(direction==3)
  {
	lock_acquire(lock_3);
	lock_acquire(lock_2);
	lock_acquire(lock_1);
	inQuadrant(3);
	inQuadrant(2);
	inQuadrant(1);
	leaveIntersection();
	lock_release(lock_3);
	lock_release(lock_2);
	lock_release(lock_1);
	
   }
lock_release(C_lock);
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}
