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

static void
_twin_label_query_geometry (twin_label_t *label)
{
    twin_path_t		*path = twin_path_create ();
    twin_text_metrics_t	m;
    
    label->widget.preferred.width = twin_fixed_to_int (label->label.font_size) * 2;
    label->widget.preferred.height = twin_fixed_to_int (label->label.font_size) * 2;
    if (path)
    {
	twin_path_set_font_size (path, label->label.font_size);
	twin_path_set_font_style (path, label->label.font_style);
	twin_text_metrics_utf8 (path, label->label.label, &m);
	label->widget.preferred.width += twin_fixed_to_int (m.width);
	twin_path_destroy (path);
    }
}

static void
_twin_label_paint (twin_label_t *label)
{
    twin_path_t		*path = twin_path_create ();
    twin_text_metrics_t	m;
    twin_coord_t	w = _twin_widget_width(label);
    twin_coord_t	h = _twin_widget_height(label);

    if (path)
    {
	twin_fixed_t	wf = twin_int_to_fixed (w);
	twin_fixed_t	hf = twin_int_to_fixed (h);
	twin_fixed_t	x, y;

	twin_path_set_font_size (path, label->label.font_size);
	twin_path_set_font_style (path, label->label.font_style);
	twin_text_metrics_utf8 (path, label->label.label, &m);
	y = (hf - (m.ascent + m.descent)) / 2 + m.ascent + label->label.offset.y;
	switch (label->label.align) {
	case TwinAlignLeft:
	    x = label->label.font_size / 2;
	    break;
	case TwinAlignCenter:
	    x = (wf - m.width) / 2;
	    break;
	case TwinAlignRight:
	    x = wf - label->label.font_size / 2 - m.width;
	    break;
	}
	x += label->label.offset.x;
	twin_path_move (path, x, y);
	twin_path_utf8 (path, label->label.label);
	twin_paint_path (label->widget.window->pixmap, label->label.foreground, path);
	twin_path_destroy (path);
    }
}

twin_dispatch_result_t
_twin_label_dispatch (twin_widget_t *widget, twin_event_t *event)
{
    twin_label_t    *label = (twin_label_t *) widget;

    if (_twin_widget_dispatch (widget, event) == TwinDispatchDone)
	return TwinDispatchDone;
    switch (event->kind) {
    case TwinEventPaint:
	_twin_label_paint (label);
	break;
    case TwinEventQueryGeometry:
	_twin_label_query_geometry (label);
	break;
    default:
	break;
    }
    return TwinDispatchContinue;
}

void
twin_label_set (twin_label_t	*label,
		const char	*value,
		twin_argb32_t	foreground,
		twin_fixed_t	font_size,
		twin_style_t	font_style)
{
    if (value)
    {
	char    *new = malloc (strlen (value) + 1);

	if (new)
	{
	    if (label->label.label)
		free (label->label.label);
	    label->label.label = new;
	    strcpy (label->label.label, value);
	}
    }
    label->label.font_size = font_size;
    label->label.font_style = font_style;
    label->label.foreground = foreground;
    _twin_widget_queue_layout ((twin_widget_t *) label);
}

void
_twin_label_init (twin_label_t		*label,
		  twin_box_t		*parent,
		  const char		*value,
		  twin_argb32_t		foreground,
		  twin_fixed_t		font_size,
		  twin_style_t		font_style,
		  twin_dispatch_proc_t	dispatch)
{
    static const twin_widget_layout_t	preferred = { 0, 0, 1, 1 };
    _twin_widget_init ((twin_widget_t *) label, parent, 0, preferred, dispatch);
    label->label.label = NULL;
    label->label.offset.x = 0;
    label->label.offset.y = 0;
    label->label.align = TwinAlignCenter;
    twin_label_set (label, value, foreground, font_size, font_style);
}


twin_label_t *
twin_label_create (twin_box_t	    *parent,
		   const char	    *value,
		   twin_argb32_t    foreground,
		   twin_fixed_t	    font_size,
		   twin_style_t	    font_style)
{
    twin_label_t    *label = malloc (sizeof (twin_label_t));

    if (!label)
	return 0;
    _twin_label_init (label, parent, value, foreground, 
		      font_size, font_style, _twin_label_dispatch);
    return label;
}

