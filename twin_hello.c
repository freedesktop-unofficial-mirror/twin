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

#include "twin_hello.h"
#include <time.h>
#include <string.h>

static twin_time_t
_twin_hello_timeout (twin_time_t now, void *closure)
{
    twin_label_t    *labelb = closure;
    time_t	    secs = time (0);
    char	    *t = ctime(&secs);

    *strchr(t, '\n') = '\0';
    twin_label_set (labelb, t,
		    0xff008000,
		    twin_int_to_fixed (12),
		    TwinStyleOblique);
    return 1000;
}

void
twin_hello_start (twin_screen_t *screen, const char *name, int x, int y, int w, int h)
{
    twin_window_t   *window = twin_window_create (screen, name, TWIN_ARGB32,
						  TwinWindowApplication,
						  x, y, w, h);
    twin_toplevel_t *top = window->toplevel;
    twin_label_t    *labela = twin_label_create (top,
						name,
						0xff000080,
						twin_int_to_fixed (12),
						TwinStyleRoman);
    twin_widget_t   *widget = twin_widget_create (top,
						  0xff800000,
						  1, 2, 0, 0);
    twin_label_t    *labelb = twin_label_create (top,
						 name,
						 0xff008000,
						 twin_int_to_fixed (12),
						 TwinStyleOblique);
    twin_button_t   *button = twin_button_create (top,
						  "Button",
						  0xff800000,
						  twin_int_to_fixed (18),
						  TwinStyleBold|TwinStyleOblique);
    twin_widget_set ((twin_widget_t *) labela, 0xc0c0c0c0);
    (void) widget;
    twin_widget_set ((twin_widget_t *) labelb, 0xc0c0c0c0);
    twin_widget_set ((twin_widget_t *) button, 0xc0808080);
    twin_window_show (window);
    twin_set_timeout (_twin_hello_timeout, 1000, labelb);
}
