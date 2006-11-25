/*
 * $Id$
 *
 * Copyright © 2004 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _TWIN_DEMOSPLINE_H_
#define _TWIN_DEMOSPLINE_H_

#include <twin.h>

#define NPT 4

typedef struct _twin_demospline_part {
    twin_point_t    points[NPT];
    int		    which;
    twin_fixed_t    line_width;
    twin_cap_t	    cap_style;
} twin_demospline_part_t;

typedef struct _twin_demospline {
    twin_widget_part_t	    widget;
    twin_demospline_part_t  demospline;
} twin_demospline_t;

void
_twin_demospline_paint (twin_demospline_t *demospline);

twin_dispatch_result_t
_twin_demospline_dispatch (twin_widget_t *widget, twin_event_t *event);
    
void
_twin_demospline_init (twin_demospline_t	*demospline, 
		       twin_box_t		*parent,
		       twin_dispatch_proc_t	dispatch);

twin_demospline_t *
twin_demospline_create (twin_box_t *parent);

void
twin_demospline_start (twin_screen_t *screen, const char *name, int x, int y, int w, int h);

#endif /* _TWIN_DEMOLINE_H_ */
