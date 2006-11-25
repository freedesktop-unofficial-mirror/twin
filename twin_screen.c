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

twin_screen_t *
twin_screen_create (twin_coord_t	width,
		    twin_coord_t	height, 
		    twin_put_begin_t	put_begin,
		    twin_put_span_t	put_span,
		    void		*closure)
{
    twin_screen_t   *screen = malloc (sizeof (twin_screen_t));
    if (!screen)
	return 0;
    screen->top = 0;
    screen->bottom = 0;
    screen->width = width;
    screen->height = height;
    screen->damage.left = screen->damage.right = 0;
    screen->damage.top = screen->damage.bottom = 0;
    screen->damaged = NULL;
    screen->damaged_closure = NULL;
    screen->disable = 0;
    screen->background = 0;
    screen->put_begin = put_begin;
    screen->put_span = put_span;
    screen->closure = closure;

    screen->button_x = screen->button_y = -1;
    return screen;
}

void
twin_screen_destroy (twin_screen_t *screen)
{
    while (screen->bottom)
	twin_pixmap_hide (screen->bottom);
    free (screen);
}

void
twin_screen_register_damaged (twin_screen_t *screen, 
			      void (*damaged) (void *),
			      void *closure)
{
    screen->damaged = damaged;
    screen->damaged_closure = closure;
}

void
twin_screen_enable_update (twin_screen_t *screen)
{
    if (--screen->disable == 0)
    {
	if (screen->damage.left < screen->damage.right &&
	    screen->damage.top < screen->damage.bottom)
	{
	    if (screen->damaged)
		(*screen->damaged) (screen->damaged_closure);
	}
    }
}

void
twin_screen_disable_update (twin_screen_t *screen)
{
    screen->disable++;
}

#include <stdio.h>

void
twin_screen_damage (twin_screen_t *screen,
		    twin_coord_t left, twin_coord_t top,
		    twin_coord_t right, twin_coord_t bottom)
{
    if (screen->damage.left == screen->damage.right)
    {
	screen->damage.left = left;
	screen->damage.right = right;
	screen->damage.top = top;
	screen->damage.bottom = bottom;
    }
    else
    {
	if (left < screen->damage.left)
	    screen->damage.left = left;
	if (top < screen->damage.top)
	    screen->damage.top = top;
	if (screen->damage.right < right)
	    screen->damage.right = right;
	if (screen->damage.bottom < bottom)
	    screen->damage.bottom = bottom;
    }
    if (screen->damaged && !screen->disable)
	(*screen->damaged) (screen->damaged_closure);
}

void
twin_screen_resize (twin_screen_t *screen, 
		    twin_coord_t width, twin_coord_t height)
{
    screen->width = width;
    screen->height = height;
    twin_screen_damage (screen, 0, 0, screen->width, screen->height);
}

twin_bool_t
twin_screen_damaged (twin_screen_t *screen)
{
    return (screen->damage.left < screen->damage.right &&
	    screen->damage.top < screen->damage.bottom);
}

void
twin_screen_update (twin_screen_t *screen)
{
    twin_coord_t	left = screen->damage.left;
    twin_coord_t	top = screen->damage.top;
    twin_coord_t	right = screen->damage.right;
    twin_coord_t	bottom = screen->damage.bottom;
    
    if (!screen->disable && left < right && top < bottom)
    {
	twin_argb32_t	*span;
        twin_pixmap_t	*p;
	twin_coord_t	y;
	twin_coord_t	width = right - left;

	screen->damage.left = screen->damage.right = 0;
	screen->damage.top = screen->damage.bottom = 0;
	/* XXX what is the maximum number of lines? */
	span = malloc (width * sizeof (twin_argb32_t));
	if (!span)
	    return;
	
	if (screen->put_begin)
	    (*screen->put_begin) (left, top, right, bottom, screen->closure);
	for (y = top; y < bottom; y++)
	{
	    if (screen->background)
	    {
		twin_pointer_t  dst;
		twin_source_u	src;
		twin_coord_t	p_left;
		twin_coord_t	m_left;
		twin_coord_t	p_this;
		twin_coord_t	p_width = screen->background->width;
		twin_coord_t	p_y = y % screen->background->height;
		
		for (p_left = left; p_left < right; p_left += p_this)
		{
		    dst.argb32 = span + (p_left - left);
		    m_left = p_left % p_width;
		    p_this = p_width - m_left;
		    if (p_left + p_this > right)
			p_this = right - p_left;
		    src.p = twin_pixmap_pointer (screen->background,
						 m_left, p_y);
		    _twin_argb32_source_argb32 (dst, src, p_this);
		}
	    }
	    else
		memset (span, 0xff, width * sizeof (twin_argb32_t));
	    for (p = screen->bottom; p; p = p->up)
	    {
		twin_pointer_t  dst;
		twin_source_u	src;
		twin_coord_t	p_left, p_right;
		
		/* bounds check in y */
		if (y < p->y)
		    continue;
		if (p->y + p->height <= y)
		    continue;
		/* bounds check in x*/
		p_left = left;
		if (p_left < p->x)
		    p_left = p->x;
		p_right = right;
		if (p_right > p->x + p->width)
		    p_right = p->x + p->width;
		if (p_left >= p_right)
		    continue;
		dst.argb32 = span + (p_left - left);
		src.p = twin_pixmap_pointer (p, p_left - p->x, y - p->y);
		if (p->format == TWIN_RGB16)
		    _twin_rgb16_source_argb32 (dst, src, p_right - p_left);
		else
		    _twin_argb32_over_argb32 (dst, src, p_right - p_left);
	    }
	    (*screen->put_span) (left, y, right, span, screen->closure);
	}
	free (span);
    }
}

void
twin_screen_set_active (twin_screen_t *screen, twin_pixmap_t *pixmap)
{
    twin_event_t    ev;
    twin_pixmap_t   *old = screen->active;
    screen->active = pixmap;
    if (old)
    {
	ev.kind = TwinEventDeactivate;
	twin_pixmap_dispatch (old, &ev);
    }
    if (pixmap)
    {
	ev.kind = TwinEventActivate;
	twin_pixmap_dispatch (pixmap, &ev);
    }
}

twin_pixmap_t *
twin_screen_get_active (twin_screen_t *screen)
{
    return screen->active;
}

void
twin_screen_set_background (twin_screen_t *screen, twin_pixmap_t *pixmap)
{
    if (screen->background)
	twin_pixmap_destroy (screen->background);
    screen->background = pixmap;
    twin_screen_damage (screen, 0, 0, screen->width, screen->height);
}

twin_pixmap_t *
twin_screen_get_background (twin_screen_t *screen)
{
    return screen->background;
}

twin_bool_t
twin_screen_dispatch (twin_screen_t *screen,
		      twin_event_t  *event)
{
    twin_pixmap_t   *pixmap;
    
    switch (event->kind) {
    case TwinEventMotion:
    case TwinEventButtonDown:
    case TwinEventButtonUp:
	pixmap = screen->pointer;
	if (!pixmap)
	{
	    for (pixmap = screen->top; pixmap; pixmap = pixmap->down)
		if (!twin_pixmap_transparent (pixmap,
					      event->u.pointer.screen_x,
					      event->u.pointer.screen_y))
		{
		    break;
		}
	    if (event->kind == TwinEventButtonDown)
		screen->pointer = pixmap;
	}
	if (event->kind == TwinEventButtonUp)
	    screen->pointer = NULL;
	if (pixmap)
	{
	    event->u.pointer.x = event->u.pointer.screen_x - pixmap->x;
	    event->u.pointer.y = event->u.pointer.screen_y - pixmap->y;
	}
	break;
    case TwinEventKeyDown:
    case TwinEventKeyUp:
    case TwinEventUcs4:
	pixmap = screen->active;
	break;
    default:
	pixmap = NULL;
	break;
    }
    if (pixmap)
	return twin_pixmap_dispatch (pixmap, event);
    return TWIN_FALSE;
}
