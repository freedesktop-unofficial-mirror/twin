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

twin_pixmap_t *
twin_pixmap_create (twin_format_t   format,
		    twin_coord_t    width,
		    twin_coord_t    height)
{
    twin_coord_t    stride = twin_bytes_per_pixel (format) * width;
    twin_area_t	    space = (twin_area_t) stride * height;
    twin_area_t	    size = sizeof (twin_pixmap_t) + space;
    twin_pixmap_t   *pixmap = malloc (size);
    if (!pixmap)
	return 0;
    pixmap->screen = 0;
    pixmap->up = 0;
    pixmap->down = 0;
    pixmap->x = pixmap->y = 0;
    pixmap->format = format;
    pixmap->width = width;
    pixmap->height = height;
    pixmap->clip.left = pixmap->clip.top = 0;
    pixmap->clip.right = pixmap->width;
    pixmap->clip.bottom = pixmap->height;
    pixmap->stride = stride;
    pixmap->disable = 0;
    pixmap->p.v = pixmap + 1;
    memset (pixmap->p.v, '\0', space);
    return pixmap;
}

twin_pixmap_t *
twin_pixmap_create_const (twin_format_t	    format,
			  twin_coord_t	    width,
			  twin_coord_t	    height,
			  twin_coord_t	    stride,
			  twin_pointer_t    pixels)
{
    twin_pixmap_t   *pixmap = malloc (sizeof (twin_pixmap_t));
    if (!pixmap)
	return 0;
    pixmap->screen = 0;
    pixmap->up = 0;
    pixmap->down = 0;
    pixmap->x = pixmap->y = 0;
    pixmap->format = format;
    pixmap->width = width;
    pixmap->height = height;
    pixmap->stride = stride;
    pixmap->disable = 0;
    pixmap->p = pixels;
    return pixmap;
}

void
twin_pixmap_destroy (twin_pixmap_t *pixmap)
{
    if (pixmap->screen)
	twin_pixmap_hide (pixmap);
    free (pixmap);
}

void
twin_pixmap_show (twin_pixmap_t	*pixmap, 
		  twin_screen_t	*screen,
		  twin_pixmap_t	*lower)
{
    if (pixmap->disable)
	twin_screen_disable_update (screen);
    
    if (lower == pixmap)
	lower = pixmap->down;
    
    if (pixmap->screen)
	twin_pixmap_hide (pixmap);
    
    pixmap->screen = screen;
    
    if (lower)
    {
	pixmap->down = lower;
	pixmap->up = lower->up;
	lower->up = pixmap;
	if (!pixmap->up)
	    screen->top = pixmap;
    }
    else
    {
	pixmap->down = NULL;
	pixmap->up = screen->bottom;
	screen->bottom = pixmap;
	if (!pixmap->up)
	    screen->top = pixmap;
    }

    twin_pixmap_damage (pixmap, 0, 0, pixmap->width, pixmap->height);
}

void
twin_pixmap_hide (twin_pixmap_t *pixmap)
{
    twin_screen_t   *screen = pixmap->screen;
    twin_pixmap_t   **up, **down;

    if (!screen)
	return;

    twin_pixmap_damage (pixmap, 0, 0, pixmap->width, pixmap->height);

    if (pixmap->up)
	down = &pixmap->up->down;
    else
	down = &screen->top;

    if (pixmap->down)
	up = &pixmap->down->up;
    else
	up = &screen->bottom;

    *down = pixmap->down;
    *up = pixmap->up;

    pixmap->screen = 0;
    pixmap->up = 0;
    pixmap->down = 0;
    if (pixmap->disable)
	twin_screen_enable_update (screen);
}

twin_pointer_t
twin_pixmap_pointer (twin_pixmap_t *pixmap, twin_coord_t x, twin_coord_t y)
{
    twin_pointer_t  p;

    p.b = (pixmap->p.b + 
	   y * pixmap->stride + 
	   x * twin_bytes_per_pixel(pixmap->format));
    return p;
}

void
twin_pixmap_enable_update (twin_pixmap_t *pixmap)
{
    if (--pixmap->disable == 0)
    {
	if (pixmap->screen)
	    twin_screen_enable_update (pixmap->screen);
    }
}

void
twin_pixmap_disable_update (twin_pixmap_t *pixmap)
{
    if (pixmap->disable++ == 0)
    {
	if (pixmap->screen)
	    twin_screen_disable_update (pixmap->screen);
    }
}

void
twin_pixmap_clip (twin_pixmap_t *pixmap,
		  twin_coord_t	left,	twin_coord_t top,
		  twin_coord_t	right,	twin_coord_t bottom)
{
    if (left > pixmap->clip.left)	pixmap->clip.left = left;
    if (top > pixmap->clip.top)		pixmap->clip.top = top;
    if (right < pixmap->clip.right)	pixmap->clip.right = right;
    if (bottom < pixmap->clip.bottom)	pixmap->clip.bottom = bottom;
    if (pixmap->clip.left >= pixmap->clip.right)
	pixmap->clip.right = pixmap->clip.left = 0;
    if (pixmap->clip.top >= pixmap->clip.bottom)
	pixmap->clip.bottom = pixmap->clip.top = 0;
}

void
twin_pixmap_set_clip (twin_pixmap_t *pixmap, twin_rect_t clip)
{
    twin_pixmap_clip (pixmap, 
		      clip.left  + pixmap->clip.left,
		      clip.top   + pixmap->clip.top,
		      clip.right + pixmap->clip.left,
		      clip.bottom+ pixmap->clip.top);
}

twin_rect_t
twin_pixmap_current_clip (twin_pixmap_t *pixmap)
{
    return pixmap->clip;
}

void
twin_pixmap_restore_clip (twin_pixmap_t *pixmap, twin_rect_t rect)
{
    pixmap->clip = rect;
}

void
twin_pixmap_reset_clip (twin_pixmap_t *pixmap)
{
    pixmap->clip.left = 0;		pixmap->clip.top = 0;
    pixmap->clip.right = pixmap->width;	pixmap->clip.bottom = pixmap->height;
}

void
twin_pixmap_damage (twin_pixmap_t   *pixmap,
		    twin_coord_t    left,	twin_coord_t top,
		    twin_coord_t    right,	twin_coord_t bottom)
{
    if (pixmap->screen)
	twin_screen_damage (pixmap->screen,
			    left + pixmap->x,
			    top + pixmap->y,
			    right + pixmap->x,
			    bottom + pixmap->y);
}

static twin_argb32_t
_twin_pixmap_fetch (twin_pixmap_t *pixmap, twin_coord_t x, twin_coord_t y)
{
    twin_pointer_t  p = twin_pixmap_pointer (pixmap, x - pixmap->x, y - pixmap->y);

    if (pixmap->x <= x && x < pixmap->x + pixmap->width &&
	pixmap->y <= y && y < pixmap->y + pixmap->height)
    {
	switch (pixmap->format) {
	case TWIN_A8:
	    return *p.a8 << 24;
	case TWIN_RGB16:
	    return twin_rgb16_to_argb32 (*p.rgb16);
	case TWIN_ARGB32:
	    return *p.argb32;
	}
    }
    return 0;
}

twin_bool_t
twin_pixmap_transparent (twin_pixmap_t *pixmap, twin_coord_t x, twin_coord_t y)
{
    return (_twin_pixmap_fetch (pixmap, x, y) >> 24) == 0;
}

void
twin_pixmap_move (twin_pixmap_t *pixmap, twin_coord_t x, twin_coord_t y)
{
    twin_pixmap_damage (pixmap, 0, 0, pixmap->width, pixmap->height);
    pixmap->x = x;
    pixmap->y = y;
    twin_pixmap_damage (pixmap, 0, 0, pixmap->width, pixmap->height);
}

twin_bool_t
twin_pixmap_dispatch (twin_pixmap_t *pixmap, twin_event_t *event)
{
    if (pixmap->toplevel)
	return _twin_toplevel_dispatch ((twin_widget_t *) pixmap->toplevel, event) == TwinDispatchDone;
    return TWIN_FALSE;
}

