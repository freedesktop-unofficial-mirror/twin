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
    default:
	break;
    }
    return _twin_box_dispatch (&toplevel->box.widget, &ev);
}

static twin_bool_t
_twin_toplevel_event (twin_window_t   *window,
		      twin_event_t    *event)
{
    twin_toplevel_t   *toplevel = window->client_data;

    return (*toplevel->box.widget.dispatch) (&toplevel->box.widget, event) == TwinDispatchDone;
}

static void
_twin_toplevel_draw (twin_window_t    *window)
{
    twin_toplevel_t   *toplevel = window->client_data;
    twin_event_t	event;

    event.kind = TwinEventPaint;
    (*toplevel->box.widget.dispatch) (&toplevel->box.widget, &event);
}

static void
_twin_toplevel_destroy (twin_window_t *window)
{
    twin_toplevel_t   *toplevel = window->client_data;
    twin_event_t	event;

    event.kind = TwinEventDestroy;
    (*toplevel->box.widget.dispatch) (&toplevel->box.widget, &event);
}

void
_twin_toplevel_init (twin_toplevel_t	    *toplevel,
		     twin_dispatch_proc_t   dispatch,
		     twin_window_t	    *window,
		     const char		    *name)
{
    twin_rect_t	extents;

    twin_window_set_name (window, name);
    extents.left = 0;
    extents.top = 0;
    extents.right = window->client.right - window->client.left;
    extents.bottom =window->client.bottom - window->client.top;
    window->draw = _twin_toplevel_draw;
    window->destroy = _twin_toplevel_destroy;
    window->event = _twin_toplevel_event;
    window->client_data = toplevel;
    _twin_box_init (&toplevel->box, 0, window, TwinBoxVert, dispatch);
}

twin_toplevel_t *
twin_toplevel_create (twin_screen_t	    *screen,
		      twin_format_t	    format,
		      twin_window_style_t   style,
		      twin_coord_t	    x,
		      twin_coord_t	    y,
		      twin_coord_t	    width,
		      twin_coord_t	    height,
		      const char	    *name)
{
    twin_toplevel_t *toplevel;
    twin_window_t   *window = twin_window_create (screen, format, style,
						  x, y, width, height);
    
    if (!window)
	return 0;
    toplevel = malloc (sizeof (twin_toplevel_t) + strlen (name) + 1);
    if (!toplevel)
    {
	twin_window_destroy (window);
	return 0;
    }
    _twin_toplevel_init (toplevel, _twin_toplevel_dispatch, window, name);
    return toplevel;
}

static twin_bool_t
_twin_toplevel_paint (void *closure)
{
    twin_toplevel_t *toplevel = closure;
    twin_event_t    ev;

    ev.kind = TwinEventPaint;
    (*toplevel->box.widget.dispatch) (&toplevel->box.widget, &ev);
    return TWIN_FALSE;
}

void
_twin_toplevel_queue_paint (twin_widget_t *widget)
{
    twin_toplevel_t *toplevel = (twin_toplevel_t *) widget;

    if (!toplevel->box.widget.paint)
    {
	toplevel->box.widget.paint = TWIN_TRUE;
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
    twin_window_t   *window = toplevel->box.widget.window;

    ev.kind = TwinEventQueryGeometry;
    (*toplevel->box.widget.dispatch) (&toplevel->box.widget, &ev);
    ev.kind = TwinEventConfigure;
    ev.u.configure.extents.left = 0;
    ev.u.configure.extents.top = 0;
    ev.u.configure. extents.right = window->client.right - window->client.left;
    ev.u.configure.extents.bottom =window->client.bottom - window->client.top;
    (*toplevel->box.widget.dispatch) (&toplevel->box.widget, &ev);
    return TWIN_FALSE;
}

void
_twin_toplevel_queue_layout (twin_widget_t *widget)
{
    twin_toplevel_t *toplevel = (twin_toplevel_t *) widget;

    if (!toplevel->box.widget.layout)
    {
	toplevel->box.widget.layout = TWIN_TRUE;
	twin_set_work (_twin_toplevel_layout,
		       TWIN_WORK_LAYOUT,
		       toplevel);
	_twin_toplevel_queue_paint (widget);
    }
}

void
twin_toplevel_show (twin_toplevel_t *toplevel)
{
    _twin_toplevel_layout (toplevel);
    _twin_toplevel_paint (toplevel);
    twin_window_show (toplevel->box.widget.window);
}

