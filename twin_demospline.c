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

#include <twin_demospline.h>
#include <twinint.h>
#include <stdio.h>

#define D(x) twin_double_to_fixed(x)

#define _twin_demospline_pixmap(demospline)   ((demospline)->widget.window->pixmap)

void
_twin_demospline_paint (twin_demospline_t *demospline)
{
    twin_path_t	*path;
    int		i;

    path = twin_path_create ();
    twin_path_set_cap_style (path, demospline->demospline.cap_style);
    twin_path_move (path, demospline->demospline.points[0].x, demospline->demospline.points[0].y);
    twin_path_curve (path, 
		     demospline->demospline.points[1].x, demospline->demospline.points[1].y,
		     demospline->demospline.points[2].x, demospline->demospline.points[2].y,
		     demospline->demospline.points[3].x, demospline->demospline.points[3].y);
    twin_paint_stroke (_twin_demospline_pixmap(demospline), 0xff404040, path,
		       demospline->demospline.line_width);
    twin_path_set_cap_style (path, TwinCapButt);
    twin_paint_stroke (_twin_demospline_pixmap(demospline), 0xffffff00, path,
		       twin_int_to_fixed (2));
    
    twin_path_empty (path);
    twin_path_move (path, demospline->demospline.points[0].x, demospline->demospline.points[0].y);
    twin_path_draw (path, demospline->demospline.points[1].x, demospline->demospline.points[1].y);
    twin_paint_stroke (_twin_demospline_pixmap(demospline), 0xc08000c0, path,
		       twin_int_to_fixed (2));
    twin_path_empty (path);
    twin_path_move (path, demospline->demospline.points[3].x, demospline->demospline.points[3].y);
    twin_path_draw (path, demospline->demospline.points[2].x, demospline->demospline.points[2].y);
    twin_paint_stroke (_twin_demospline_pixmap(demospline), 0xc08000c0, path,
		       twin_int_to_fixed (2));
    twin_path_empty (path);
    for (i = 0; i < NPT; i++)
    {
	twin_path_empty (path);
	twin_path_circle (path, demospline->demospline.points[i].x, demospline->demospline.points[i].y,
			  twin_int_to_fixed (10));
	twin_paint_path (_twin_demospline_pixmap(demospline), 0x40004020, path);
    }
    twin_path_destroy (path);
}

static twin_dispatch_result_t
_twin_demospline_update_pos (twin_demospline_t *demospline, twin_event_t *event)
{
    if (demospline->demospline.which < 0)
	return TwinDispatchContinue;
    demospline->demospline.points[demospline->demospline.which].x = twin_int_to_fixed (event->u.pointer.x);
    demospline->demospline.points[demospline->demospline.which].y = twin_int_to_fixed (event->u.pointer.y);
    _twin_widget_queue_paint ((twin_widget_t *) demospline);
    return TwinDispatchDone;
}

#define twin_fixed_abs(f)   ((f) < 0 ? -(f) : (f))

static int
_twin_demospline_hit (twin_demospline_t *demospline, twin_fixed_t x, twin_fixed_t y)
{
    int	i;

    for (i = 0; i < NPT; i++)
	if (twin_fixed_abs (x - demospline->demospline.points[i].x) < demospline->demospline.line_width / 2 &&
	    twin_fixed_abs (y - demospline->demospline.points[i].y) < demospline->demospline.line_width / 2)
	    return i;
    return -1;
}

twin_dispatch_result_t
_twin_demospline_dispatch (twin_widget_t *widget, twin_event_t *event)
{
    twin_demospline_t    *demospline = (twin_demospline_t *) widget;

    if (_twin_widget_dispatch (widget, event) == TwinDispatchDone)
	return TwinDispatchDone;
    switch (event->kind) {
    case TwinEventPaint:
	_twin_demospline_paint (demospline);
	break;
    case TwinEventButtonDown:
	demospline->demospline.which = _twin_demospline_hit (demospline,
					      twin_int_to_fixed (event->u.pointer.x),
					      twin_int_to_fixed (event->u.pointer.y));
	return _twin_demospline_update_pos (demospline, event);
	break;
    case TwinEventMotion:
	return _twin_demospline_update_pos (demospline, event);
	break;
    case TwinEventButtonUp:
	if (demospline->demospline.which < 0)
	    return TwinDispatchContinue;
	_twin_demospline_update_pos (demospline, event);
	demospline->demospline.which = -1;
	return TwinDispatchDone;
	break;
    default:
	break;
    }
    return TwinDispatchContinue;
}

void
_twin_demospline_init (twin_demospline_t		*demospline, 
		  twin_box_t		*parent,
		  twin_dispatch_proc_t	dispatch)
{
    static const twin_widget_layout_t	preferred = { 0, 0, 1, 1 };
    _twin_widget_init ((twin_widget_t *) demospline, parent, 0, preferred, dispatch);
    twin_widget_set ((twin_widget_t *) demospline, 0xffffffff);
    demospline->demospline.line_width = twin_int_to_fixed (100);
    demospline->demospline.cap_style = TwinCapRound;
    demospline->demospline.points[0].x = twin_int_to_fixed (100);
    demospline->demospline.points[0].y = twin_int_to_fixed (100);
    demospline->demospline.points[1].x = twin_int_to_fixed (300);
    demospline->demospline.points[1].y = twin_int_to_fixed (300);
    demospline->demospline.points[2].x = twin_int_to_fixed (100);
    demospline->demospline.points[2].y = twin_int_to_fixed (300);
    demospline->demospline.points[3].x = twin_int_to_fixed (300);
    demospline->demospline.points[3].y = twin_int_to_fixed (100);
}
    
twin_demospline_t *
twin_demospline_create (twin_box_t *parent)
{
    twin_demospline_t    *demospline = malloc (sizeof (twin_demospline_t));
    
    _twin_demospline_init(demospline, parent, _twin_demospline_dispatch);
    return demospline;
}

void
twin_demospline_start (twin_screen_t *screen, const char *name, int x, int y, int w, int h)
{
    twin_window_t   *window = twin_window_create (screen, name, TWIN_ARGB32,
						  TwinWindowApplication,
						  x, y, w, h);
    twin_toplevel_t *toplevel = window->toplevel;
    twin_demospline_t    *demospline = twin_demospline_create (toplevel);
    (void) demospline;
    twin_window_show (window);
}
