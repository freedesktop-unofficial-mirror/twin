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

void
_twin_box_init (twin_box_t		*box,
		twin_box_t		*parent,
		twin_window_t		*window,
		twin_box_dir_t		dir,
		twin_dispatch_proc_t	dispatch)
{
    static twin_widget_layout_t	preferred = { 0, 0, 0, 0 };
    _twin_widget_init ((twin_widget_t *) box, parent,
		       window, preferred, dispatch);
    box->box.dir = dir;
    box->box.children = NULL;
    box->box.key_focus = NULL;
    box->box.pointer_grab = NULL;
}

static twin_dispatch_result_t
_twin_box_query_geometry (twin_box_t *box)
{
    twin_widget_t	    *child;
    twin_event_t	    ev;
    twin_widget_layout_t    preferred;

    preferred.width = 0;
    preferred.height = 0;
    if (box->box.dir == TwinBoxHorz)
    {
	preferred.stretch_width = 0;
	preferred.stretch_height = 10000;
    }
    else
    {
	preferred.stretch_width = 10000;
	preferred.stretch_height = 0;
    }
    /*
     * Find preferred geometry
     */
    for (child = box->box.children; child; child = child->widget.next)
    {
	if (child->widget.layout)
	{
	    ev.kind = TwinEventQueryGeometry;
	    (*child->widget.dispatch) (child, &ev);
	}
	if (box->box.dir == TwinBoxHorz)
	{
	    preferred.width += child->widget.preferred.width;
	    preferred.stretch_width += child->widget.preferred.stretch_width;
	    if (child->widget.preferred.height > preferred.height)
		preferred.height = child->widget.preferred.height;
	    if (child->widget.preferred.stretch_height < preferred.stretch_height)
		preferred.stretch_height = child->widget.preferred.stretch_height;
	}
	else
	{
	    preferred.height += child->widget.preferred.height;
	    preferred.stretch_height += child->widget.preferred.stretch_height;
	    if (child->widget.preferred.width > preferred.width)
		preferred.width = child->widget.preferred.width;
	    if (child->widget.preferred.stretch_width < preferred.stretch_width)
		preferred.stretch_width = child->widget.preferred.stretch_width;
	}
    }
    box->widget.preferred = preferred;
    return TwinDispatchContinue;
}

static twin_dispatch_result_t
_twin_box_configure (twin_box_t *box)
{
    twin_coord_t    width = _twin_widget_width (box);
    twin_coord_t    height = _twin_widget_height (box);
    twin_coord_t    actual;
    twin_coord_t    pref;
    twin_coord_t    delta;
    twin_coord_t    delta_remain;
    twin_coord_t    stretch = 0;
    twin_coord_t    pos = 0;
    twin_widget_t   *child;
    
    if (box->box.dir == TwinBoxHorz)
    {
	stretch = box->widget.preferred.stretch_width;
	actual = width;
	pref = box->widget.preferred.width;
    }
    else
    {
	stretch = box->widget.preferred.stretch_height;
	actual = height;
	pref = box->widget.preferred.height;
    }
    if (!stretch) stretch = 1;
    delta = delta_remain = actual - pref;
    for (child = box->box.children; child; child = child->widget.next)
    {
	twin_event_t	ev;
	twin_coord_t    stretch_this;
	twin_coord_t    delta_this;
	twin_rect_t	extents;
	
	if (box->box.dir == TwinBoxHorz)
	    stretch_this = child->widget.preferred.stretch_width;
	else
	    stretch_this = child->widget.preferred.stretch_height;
	if (!child->widget.next && stretch_this)
	    delta_this = delta_remain;
	else
	    delta_this = delta * stretch_this / stretch;
	if (delta_remain < 0)
	{
	    if (delta_this < delta_remain)
		delta_this = delta_remain;
	}
	else
	{
	    if (delta_this > delta_remain)
		delta_this = delta_remain;
	}
	delta_remain -= delta_this;
	if (box->box.dir == TwinBoxHorz)
	{
	    twin_coord_t    child_w = child->widget.preferred.width;
	    extents.top = 0;
	    extents.bottom = height;
	    extents.left = pos;
	    pos = extents.right = pos + child_w + delta_this;
	}
	else
	{
	    twin_coord_t    child_h = child->widget.preferred.height;
	    extents.left = 0;
	    extents.right = width;
	    extents.top = pos;
	    pos = extents.bottom = pos + child_h + delta_this;
	}
	if (extents.left != ev.u.configure.extents.left ||
	    extents.top != ev.u.configure.extents.top ||
	    extents.right != ev.u.configure.extents.right ||
	    extents.bottom != ev.u.configure.extents.bottom)
	{
	    ev.kind = TwinEventConfigure;
	    ev.u.configure.extents = extents;
	    (*child->widget.dispatch) (child, &ev);
	}
    }
    return TwinDispatchContinue;
}

static twin_widget_t *
_twin_box_xy_to_widget (twin_box_t *box, twin_coord_t x, twin_coord_t y)
{
    twin_widget_t   *child;

    for (child = box->box.children; child; child = child->widget.next)
    {
	if (child->widget.extents.left <= x && x < child->widget.extents.right &&
	    child->widget.extents.top <= y && y < child->widget.extents.bottom)
	    return child;
    }
    return NULL;
}

twin_dispatch_result_t
_twin_box_dispatch (twin_widget_t *widget, twin_event_t *event)
{
    twin_box_t	    *box = (twin_box_t *) widget;
    twin_event_t    ev;
    twin_widget_t   *child;

    if (event->kind != TwinEventPaint &&
	_twin_widget_dispatch (widget, event) == TwinDispatchDone)
	return TwinDispatchDone;
    switch (event->kind) {
    case TwinEventQueryGeometry:
	return _twin_box_query_geometry (box);
    case TwinEventConfigure:
	return _twin_box_configure (box);
    case TwinEventButtonDown:
	box->box.pointer_grab = _twin_box_xy_to_widget (box, 
						   event->u.pointer.x,
						   event->u.pointer.y);
	if (box->box.pointer_grab && box->box.pointer_grab->widget.want_focus)
	    box->box.key_focus = box->box.pointer_grab;
	/* fall through ... */
    case TwinEventButtonUp:
    case TwinEventMotion:
	if (box->box.pointer_grab)
	{
	    child = box->box.pointer_grab;
	    ev = *event;
	    ev.u.pointer.x -= child->widget.extents.left;
	    ev.u.pointer.y -= child->widget.extents.top;
	    return (*box->box.pointer_grab->widget.dispatch) (child, &ev);
	}
	break;
    case TwinEventKeyDown:
    case TwinEventKeyUp:
    case TwinEventUcs4:
	if (box->box.key_focus)
	    return (*box->box.key_focus->widget.dispatch) (box->box.key_focus, event);
	break;
    case TwinEventPaint:
	box->widget.paint = TWIN_FALSE;
	for (child = box->box.children; child; child = child->widget.next)
	    if (child->widget.paint)
	    {
		twin_pixmap_t	*pixmap = box->widget.window->pixmap;
		twin_rect_t	clip = twin_pixmap_current_clip (pixmap);

		if (child->widget.shape != TwinShapeRectangle)
		    twin_fill (child->widget.window->pixmap,
			       widget->widget.background, TWIN_SOURCE,
			       child->widget.extents.left, child->widget.extents.top,
			       child->widget.extents.right, child->widget.extents.bottom);
		twin_pixmap_set_clip (pixmap, child->widget.extents);
		child->widget.paint = TWIN_FALSE;
		(*child->widget.dispatch) (child, event);
		twin_pixmap_restore_clip (pixmap, clip);
	    }
	break;
    default:
	break;
    }
    return TwinDispatchContinue;
}

twin_box_t *
twin_box_create (twin_box_t	*parent,
		 twin_box_dir_t	dir)
{
    twin_box_t	*box = malloc (sizeof (twin_box_t));
    if (!box)
	return 0;
    _twin_box_init (box, parent, 0, dir, _twin_box_dispatch);
    return box;
}

