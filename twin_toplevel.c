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

#include "twinint.h"

static twin_bool_t
_twin_toplevel_event (twin_window_t   *window,
		      twin_event_t    *event)
{
    twin_toplevel_t   *toplevel = window->client_data;

    return (*toplevel->widget.dispatch) ((twin_widget_t *) toplevel, event) == TwinDispatchDone;
}

static void
_twin_toplevel_draw (twin_window_t    *window)
{
    twin_toplevel_t   *toplevel = window->client_data;
    twin_event_t	event;

    event.kind = TwinEventPaint;
    (*toplevel->widget.dispatch) ((twin_widget_t *) toplevel, &event);
}

static void
_twin_toplevel_destroy (twin_window_t *window)
{
    twin_toplevel_t   *toplevel = window->client_data;
    twin_event_t	event;

    event.kind = TwinEventDestroy;
    (*toplevel->widget.dispatch) ((twin_widget_t *) toplevel, &event);
}

void
_twin_toplevel_init (twin_toplevel_t	    *toplevel,
		     twin_window_t	    *window,
		     twin_box_dir_t	    dir,
		     twin_dispatch_proc_t   dispatch)
{
    _twin_box_init ((twin_box_t *) toplevel, 0, window, dir, dispatch);
}

twin_toplevel_t *
twin_toplevel_create (twin_screen_t	    *screen,
		      twin_window_t	    *window,
		      twin_box_dir_t	    dir)
{
    twin_toplevel_t *toplevel;

    toplevel = malloc (sizeof (twin_toplevel_t));
    if (!toplevel)
    {
	twin_window_destroy (window);
	return 0;
    }
    _twin_toplevel_init (toplevel, window, dir, _twin_toplevel_dispatch);
    return toplevel;
}

static twin_bool_t
_twin_toplevel_paint (void *closure)
{
    twin_toplevel_t *toplevel = closure;
    twin_event_t    ev;

    ev.kind = TwinEventPaint;
    (*toplevel->widget.dispatch) ((twin_widget_t *) toplevel, &ev);
    return TWIN_FALSE;
}

void
_twin_toplevel_queue_paint (twin_widget_t *widget)
{
    twin_toplevel_t *toplevel = (twin_toplevel_t *) widget;

    if (!toplevel->widget.paint)
    {
	toplevel->widget.paint = TWIN_TRUE;
	twin_set_work (_twin_toplevel_paint,
		       TWIN_WORK_PAINT,
		       toplevel);
	
    }
}

static twin_bool_t
_twin_toplevel_layout (void *closure)
{
    twin_toplevel_t *toplevel = closure;
    twin_event_t    ev;
    twin_window_t   *window = toplevel->widget.window;

    ev.kind = TwinEventQueryGeometry;
    (*toplevel->widget.dispatch) ((twin_widget_t *) toplevel, &ev);
    ev.kind = TwinEventConfigure;
    ev.u.configure.extents.left = 0;
    ev.u.configure.extents.top = 0;
    ev.u.configure. extents.right = window->client.right - window->client.left;
    ev.u.configure.extents.bottom =window->client.bottom - window->client.top;
    (*toplevel->widget.dispatch) ((twin_widget_t *) toplevel, &ev);
    return TWIN_FALSE;
}

void
_twin_toplevel_queue_layout (twin_widget_t *widget)
{
    twin_toplevel_t *toplevel = (twin_toplevel_t *) widget;

    if (!toplevel->widget.layout)
    {
	toplevel->widget.layout = TWIN_TRUE;
	twin_set_work (_twin_toplevel_layout,
		       TWIN_WORK_LAYOUT,
		       toplevel);
	_twin_toplevel_queue_paint (widget);
    }
}

twin_dispatch_result_t
_twin_toplevel_dispatch (twin_widget_t *widget, twin_event_t *event)
{
    twin_toplevel_t *toplevel = (twin_toplevel_t *) widget;
    twin_event_t    ev = *event;
    
    switch (ev.kind) {
    case TwinEventConfigure:
	ev.u.configure.extents.left = 0;
	ev.u.configure.extents.top = 0;
	ev.u.configure.extents.right = (event->u.configure.extents.right -
					event->u.configure.extents.left);
	ev.u.configure.extents.bottom = (event->u.configure.extents.bottom -
					 event->u.configure.extents.top);
	break;
    case TwinEventShow:
	_twin_toplevel_layout (toplevel);
	_twin_toplevel_paint (toplevel);
	break;
    default:
	break;
    }
    return _twin_box_dispatch ((twin_widget_t *) toplevel, &ev);
}

#if 0
void
twin_toplevel_show (twin_toplevel_t *toplevel)
{
    _twin_toplevel_layout (toplevel);
    _twin_toplevel_paint (toplevel);
    twin_window_show (toplevel->widget.window);
}
#endif
