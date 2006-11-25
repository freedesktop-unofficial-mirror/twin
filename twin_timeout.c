/*
 * Twin - A Tiny Window System
 * Copyright © 2004 Keith Packard <keithp@keithp.com>
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Twin Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "twinint.h"

static twin_timeout_t    *head;
static twin_time_t	    start;

static twin_order_t
_twin_timeout_order (twin_queue_t *a, twin_queue_t *b)
{
    twin_timeout_t	*at = (twin_timeout_t *) a;
    twin_timeout_t	*bt = (twin_timeout_t *) b;
    
    if (twin_time_compare (at->time, <, bt->time))
	return TWIN_BEFORE;
    if (twin_time_compare (at->time, >, bt->time))
	return TWIN_AFTER;
    return TWIN_AT;
}

static void
_twin_queue_timeout (twin_timeout_t   *timeout, twin_time_t time)
{
    timeout->time = time;
    _twin_queue_remove ((twin_queue_t **) &head, 
			   &timeout->queue);
    _twin_queue_insert ((twin_queue_t **) &head, 
			   _twin_timeout_order, &timeout->queue);
}

void
_twin_run_timeout (void)
{
    twin_time_t	now = twin_now ();
    twin_timeout_t   *timeout;
    twin_timeout_t   *first;
    twin_time_t	delay;

    first = (twin_timeout_t *) _twin_queue_set_order ((twin_queue_t **) &head);
    for (timeout = first; 
	 timeout && twin_time_compare (now, >=, timeout->time);
	 timeout = (twin_timeout_t *) timeout->queue.order)
    {
	delay = (*timeout->proc) (now, timeout->closure);
	if (delay >= 0)
	{
	    _twin_queue_remove ((twin_queue_t **) &head, 
				   &timeout->queue);
	    _twin_queue_timeout (timeout, twin_now () + delay);
	}
	else
	    _twin_queue_delete ((twin_queue_t **) &head, 
				   &timeout->queue);
    }
    _twin_queue_review_order (&first->queue);
}

twin_timeout_t *
twin_set_timeout (twin_timeout_proc_t	timeout_proc,
		  twin_time_t		delay,
		  void			*closure)
{
    twin_timeout_t	*timeout = malloc (sizeof (twin_timeout_t));

    if (!timeout)
	return 0;

    if (!start)
	start = twin_now ();
    timeout->delay = delay;
    timeout->proc = timeout_proc;
    timeout->closure = closure;
    _twin_queue_timeout (timeout, twin_now() + delay);
    return timeout;
}

void
twin_clear_timeout (twin_timeout_t *timeout)
{
    _twin_queue_delete ((twin_queue_t **) &head, &timeout->queue);
}

twin_time_t
_twin_timeout_delay (void)
{
    if (head)
    {
	twin_time_t	now = twin_now();
	if (twin_time_compare (now, >=, head->time))
	    return 0;
	return head->time - now;
    }
    return -1;
}

#include <sys/time.h>
#include <time.h>

twin_time_t
twin_now (void)
{
    struct timeval  tv;
    gettimeofday (&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
