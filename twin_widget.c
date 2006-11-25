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

static twin_path_t *
_twin_path_shape (twin_shape_t	shape,
		  twin_coord_t	left,
		  twin_coord_t	top,
		  twin_coord_t	right,
		  twin_coord_t	bottom,
		  twin_fixed_t	radius)
{
    twin_path_t	    *path = twin_path_create ();
    twin_fixed_t    x = twin_int_to_fixed (left);
    twin_fixed_t    y = twin_int_to_fixed (top);
    twin_fixed_t    w = twin_int_to_fixed (right - left);
    twin_fixed_t    h = twin_int_to_fixed (bottom - top);

    if (!path)
	return 0;
    switch (shape) {
    case TwinShapeRectangle:
	twin_path_rectangle (path, x, y, w, h);
	break;
    case TwinShapeRoundedRectangle:
	twin_path_rounded_rectangle (path, x, h, w, y, radius, radius);
	break;
    case TwinShapeLozenge:
	twin_path_lozenge  (path, x, y, w, h);
	break;
    case TwinShapeTab:
	twin_path_tab (path, x, y, w, h, radius, radius);
	break;
    case TwinShapeEllipse:
	twin_path_ellipse (path, x + w/2, y + h/2, w/2, h/2);
	break;
    }
    return path;
}

void
_twin_widget_paint_shape (twin_widget_t *widget,
			  twin_shape_t	shape,
			  twin_coord_t	left,
			  twin_coord_t	top,
			  twin_coord_t	right,
			  twin_coord_t	bottom,
			  twin_fixed_t	radius)
{
    twin_pixmap_t	*pixmap = widget->widget.window->pixmap;
    
    if (shape == TwinShapeRectangle)
	twin_fill (pixmap, widget->widget.background, TWIN_SOURCE, 
		   left, top, right, bottom);
    else
    {
	twin_path_t	*path = _twin_path_shape (shape, left, top,
						  right, bottom, radius);
	if (path)
	{
	    twin_paint_path (pixmap, widget->widget.background, path);
	    twin_path_destroy (path);
	}
    }
}

static void
_twin_widget_paint (twin_widget_t *widget)
{
    _twin_widget_paint_shape (widget, widget->widget.shape, 0, 0, 
			      _twin_widget_width (widget), 
			      _twin_widget_height (widget), widget->widget.radius);
}

twin_dispatch_result_t
_twin_widget_dispatch (twin_widget_t *widget, twin_event_t *event)
{
    switch (event->kind) {
    case TwinEventQueryGeometry:
	widget->widget.layout = TWIN_FALSE;
	if (widget->widget.copy_geom)
	{
	    twin_widget_t   *copy = widget->widget.copy_geom;
	    if (copy->widget.layout)
		(*copy->widget.dispatch) (copy, event);
	    widget->widget.preferred = copy->widget.preferred;
	    return TwinDispatchDone;
	}
	break;
    case TwinEventConfigure:
	widget->widget.extents = event->u.configure.extents;
	break;
    case TwinEventPaint:
	_twin_widget_paint (widget);
	widget->widget.paint = TWIN_FALSE;
	break;
    default:
	break;
    }
    return TwinDispatchContinue;
}

void
_twin_widget_init (twin_widget_t	*widget,
		   twin_box_t		*parent,
		   twin_window_t	*window,
		   twin_widget_layout_t	preferred,
		   twin_dispatch_proc_t	dispatch)
{
    if (parent)
    {
	twin_widget_t   **prev;

	for (prev = &parent->box.children; *prev; prev = &(*prev)->widget.next);
	widget->widget.next = *prev;
	*prev = widget;
	window = parent->widget.window;
    }
    else
	widget->widget.next = NULL;
    widget->widget.window = window;
    widget->widget.parent = parent;
    widget->widget.copy_geom = NULL;
    widget->widget.paint = TWIN_TRUE;
    widget->widget.layout = TWIN_TRUE;
    widget->widget.want_focus = TWIN_FALSE;
    widget->widget.background = 0x00000000;
    widget->widget.extents.left = widget->widget.extents.top = 0;
    widget->widget.extents.right = widget->widget.extents.bottom = 0;
    widget->widget.preferred = preferred;
    widget->widget.dispatch = dispatch;
    widget->widget.shape = TwinShapeRectangle;
    widget->widget.radius = twin_int_to_fixed (12);
    widget->widget.closure = NULL;
}

void
_twin_widget_queue_paint (twin_widget_t   *widget)
{
    while (widget->widget.parent)
    {
	if (widget->widget.paint)
	    return;
	widget->widget.paint = TWIN_TRUE;
	widget = (twin_widget_t *) widget->widget.parent;
    }
    _twin_toplevel_queue_paint (widget);
}

void
_twin_widget_queue_layout (twin_widget_t   *widget)
{
    while (widget->widget.parent)
    {
	if (widget->widget.layout)
	    return;
	widget->widget.layout = TWIN_TRUE;
	widget->widget.paint = TWIN_TRUE;
	widget = (twin_widget_t *) widget->widget.parent;
    }
    _twin_toplevel_queue_layout (widget);
}

twin_bool_t
_twin_widget_contains (twin_widget_t	*widget,
		       twin_coord_t	x,
		       twin_coord_t	y)
{
    return (0 <= x && x < _twin_widget_width(widget) && 
	    0 <= y && y < _twin_widget_height(widget)); 
}

static twin_path_t *
_twin_path_bevel_shape (twin_shape_t	shape,
			twin_bool_t	upper,
			twin_coord_t	left,
			twin_coord_t	top,
			twin_coord_t	right,
			twin_coord_t	bottom,
			twin_fixed_t	radius,
			twin_fixed_t	bw)
{
    return 0;
}

void
_twin_widget_bevel (twin_widget_t   *widget,
		    twin_fixed_t    b,
		    twin_bool_t	    down)
{
    twin_path_t		*path = twin_path_create ();
    twin_fixed_t	w = twin_int_to_fixed (_twin_widget_width (widget));
    twin_fixed_t	h = twin_int_to_fixed (_twin_widget_height (widget));
    twin_argb32_t	top_color, bot_color;
    twin_pixmap_t	*pixmap = widget->widget.window->pixmap;
    
    if (path)
    {
	if (down)
	{
	    top_color = 0x80000000;
	    bot_color = 0x80808080;
	}
	else
	{
	    top_color = 0x80808080;
	    bot_color = 0x80000000;
	}
	twin_path_move (path, 0, 0);
	twin_path_draw (path, w, 0);
	twin_path_draw (path, w-b, b);
	twin_path_draw (path, b, b);
	twin_path_draw (path, b, h-b);
	twin_path_draw (path, 0, h);
	twin_path_close (path);
	twin_paint_path (pixmap, top_color, path);
	twin_path_empty (path);
	twin_path_move (path, b, h-b);
	twin_path_draw (path, w-b, h-b);
	twin_path_draw (path, w-b, b);
	twin_path_draw (path, w, 0);
	twin_path_draw (path, w, h);
	twin_path_draw (path, 0, h);
	twin_path_close (path);
	twin_paint_path (pixmap, bot_color, path);
	twin_path_destroy (path);
    }
}

twin_widget_t *
twin_widget_create (twin_box_t	    *parent,
		    twin_argb32_t   background,
		    twin_coord_t    width,
		    twin_coord_t    height,
		    twin_stretch_t  stretch_width,
		    twin_stretch_t  stretch_height)
{
    twin_widget_t	    *widget = malloc (sizeof (twin_widget_t));
    twin_widget_layout_t    preferred;

    if (!widget)
	return NULL;
    preferred.width = width;
    preferred.height = height;
    preferred.stretch_width = stretch_width;
    preferred.stretch_height = stretch_height;
    _twin_widget_init (widget, parent, 0, preferred, _twin_widget_dispatch);
    widget->widget.background = background;
    return widget;
}
		    
void
twin_widget_set (twin_widget_t *widget, twin_argb32_t background)
{
    widget->widget.background = background;
    _twin_widget_queue_paint (widget);
}
