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

#include "twin_ttf.h"

static double pos (FT_Pos x, outline_closure_t *c)
{
    FT_Face face = c->face;

    return (double) x / (double) face->units_per_EM;
}

static void command (char cmd, outline_closure_t *c)
{
    printf ("\t'%c', ", cmd);
    c->offset++;
}

static int conv (FT_Pos x, outline_closure_t *c)
{
	return floor(64.0 * pos (x, c) + 0.5);
}

static void cpos (FT_Pos x, outline_closure_t *c)
{
    printf ("%d, ", conv(x, c));
    c->offset++;
}

static unsigned char * ucs4_to_utf8 (FT_ULong	ucs4,
				     unsigned char dest[8])
{
    int	bits;
    unsigned char *d = dest;
    
    if      (ucs4 <       0x80) {  *d++=  ucs4;                         bits= -6; }
    else if (ucs4 <      0x800) {  *d++= ((ucs4 >>  6) & 0x1F) | 0xC0;  bits=  0; }
    else if (ucs4 <    0x10000) {  *d++= ((ucs4 >> 12) & 0x0F) | 0xE0;  bits=  6; }
    else if (ucs4 <   0x200000) {  *d++= ((ucs4 >> 18) & 0x07) | 0xF0;  bits= 12; }
    else if (ucs4 <  0x4000000) {  *d++= ((ucs4 >> 24) & 0x03) | 0xF8;  bits= 18; }
    else if (ucs4 < 0x80000000) {  *d++= ((ucs4 >> 30) & 0x01) | 0xFC;  bits= 24; }
    else return 0;

    for ( ; bits >= 0; bits-= 6) {
	*d++= ((ucs4 >> bits) & 0x3F) | 0x80;
    }
    *d++ = '\0';
    return dest;
}

static void glyph (FT_GlyphSlot glyph, FT_ULong ucs4, outline_closure_t *c)
{
    FT_Pos advance = glyph->linearHoriAdvance;
    unsigned char   utf8[8];

    printf ("    /* 0x%lx (%s) */ \n", ucs4, ucs4_to_utf8 (ucs4, utf8));
 
    /* I'm very unusre about the metrics here, mostly because I'm not 100%
     * confident what twin expects in some of those fields
     *
     * twin wants: left, right, ascent, descent.
     *
     * For now, I'm setting "left" to horiBearingX though I'm not quite sure
     * what this is used for, "right" to the full horizontal advance of
     * the glyph as that's what twin uses as an advance between characters,
     * "ascent" I set to horiBearingY (unsure there too though) and for the
     * last, descent, I suppose I should use the total height minus the ascent
     * though I'm not sure how to retreive the former (bbox ?).
     *
     * Note that currently, we only support horizontal text.
     */
    cpos (glyph->metrics.horiBearingX, c); /* left, not sure about that */
    cpos (advance, c);			   /* right */
    cpos (glyph->metrics.horiBearingY, c); /* ascent, not sure either */
    cpos (0 /* XXX */, c);		   /* descent, not handled now */
    printf ("\n");
}

static int outline_moveto (const FT_Vector *to, void *user)
{
    outline_closure_t	*c = user;
    command ('m', c); cpos (to->x, c); cpos (-to->y, c);
    printf ("\n");
    return 0;
}

static int outline_lineto (const FT_Vector *to, void *user)
{
    outline_closure_t	*c = user;
    command ('l', c); cpos (to->x, c); cpos (-to->y, c);
    printf ("\n");
    return 0;
}

static int outline_conicto (const FT_Vector *control, const FT_Vector *to,
			    void *user)
{
    outline_closure_t	*c = user;
    command ('2', c); 
    cpos (control->x, c); cpos (0-control->y, c); 
    cpos (to->x, c); cpos (0-to->y, c);
    printf ("\n");
    return 0;
}

static int outline_cubicto (const FT_Vector *control1,
			    const FT_Vector *control2,
			    const FT_Vector *to,
			    void *user)
{
    outline_closure_t	*c = user;
    command ('2', c); 
    cpos (control1->x, c); cpos (0-control1->y, c); 
    cpos (control2->x, c); cpos (0-control2->y, c); 
    cpos (to->x, c); cpos (0-to->y, c);
    printf ("\n");
    return 0;
}

static const FT_Outline_Funcs outline_funcs = {
    .move_to	= outline_moveto,
    .line_to	= outline_lineto,
    .conic_to	= outline_conicto,
    .cubic_to	= outline_cubicto,
    .shift	= 0,
    .delta	= 0
};

#define UCS_PAGE_SHIFT	7
#define UCS_PER_PAGE    (1 << UCS_PAGE_SHIFT)

static int ucs_page (FT_ULong ucs4)
{
    return ucs4 >> UCS_PAGE_SHIFT;
}

static FT_ULong ucs_first_in_page (FT_ULong ucs4)
{
    return ucs4 & ~(UCS_PER_PAGE - 1);
}

static void sanitize (char *in, char *out, int first)
{
    char    c;

    while ((c = *in++))
    {
	if (('a' <= c && c <= 'z') ||
	    ('A' <= c && c <= 'Z') ||
	    ('c' == '_'))
	    ;
	else if ('0' <= c && c <= '9')
	{
	    if (first)
		*out++ = '_';
	}
	else
	    c = '_';
	*out++ = c;
	first = 0;
    }
    *out++ = '\0';
}

static char * facename (FT_Face face)
{
    char    *family = face->family_name;
    char    *style = face->style_name;
    char    *name = malloc (1 + strlen (family) + 1 + strlen (style) + 1);
    
    sanitize (family, name, 1);
    strcat (name, "_");
    sanitize (style, name + strlen (name), 0);
    return name;
}

#define MAX_UCS4    0x1000000

static int convert_font (char *in_name, int id)
{
    FT_Library	    ftLibrary;
    FT_Face	    face;
    FT_UInt	    gindex;
    FT_ULong	    ucs4;
    FT_Int32	    load_flags;
    outline_closure_t	closure;
    FT_ULong	    min_ucs4, max_ucs4;
    int		    *offsets;
    int		    ncharmap;
    
    if (FT_Init_FreeType (&ftLibrary))
	return 0;
    
    if (FT_New_Face (ftLibrary, in_name, id, &face))
	return 0;

    if (FT_Select_Charmap (face, ft_encoding_unicode))
	return 0;

    load_flags = FT_LOAD_NO_SCALE|FT_LOAD_LINEAR_DESIGN;

    closure.face = face;
    closure.offset = 0;
    
    offsets = calloc (face->num_glyphs + 1, sizeof (int));
    
    min_ucs4 = 0xffffff;
    max_ucs4 = 0;
    printf ("/* Derived from %s */\n\n", in_name);
    printf ("#include \"twin.h\"\n\n");
    printf ("static const signed char outlines[] = {\n");
    for (ucs4 = FT_Get_First_Char (face, &gindex); 
	 gindex != 0 && ucs4 < MAX_UCS4;
	 ucs4 = FT_Get_Next_Char (face, ucs4, &gindex))
    {
	if (FT_Load_Glyph (face, gindex, load_flags) != 0)
	    continue;
	if (ucs4 < min_ucs4)
	    min_ucs4 = ucs4;
	if (ucs4 > max_ucs4)
	    max_ucs4 = ucs4;
	//offsets[gindex] = closure.offset + 1;
	offsets[gindex] = closure.offset;
	glyph (face->glyph, ucs4, &closure);
	FT_Outline_Decompose (&face->glyph->outline, &outline_funcs, &closure);
	command ('e', &closure); printf ("\n");
    }
    printf ("};\n\n");
    printf ("static const twin_charmap_t charmap[] = {\n");
    ncharmap = 0;
    for (ucs4 = FT_Get_First_Char (face, &gindex); 
	 gindex != 0 && ucs4 < MAX_UCS4;
	 ucs4 = FT_Get_Next_Char (face, ucs4, &gindex))
    {
	FT_ULong	page = ucs_first_in_page (ucs4);
	FT_ULong	off;

	printf ("    { 0x%04x, {\n", ucs_page (page));
	for (off = 0; off < UCS_PER_PAGE; off++)
	{
	    FT_UInt g = FT_Get_Char_Index (face, page + off);
	    if ((off & 7) == 0)
		printf ("\t");
	    printf ("0x%04x, ", offsets[g]);
	    if ((off & 7) == 7)
		printf ("\n");
	}
	printf ("    }},\n");
	ucs4 = page + UCS_PER_PAGE - 1;
	ncharmap++;
    }
    printf ("};\n\n");
    printf ("twin_font_t twin_%s = {\n", facename (face));
    printf ("\t.type\t\t= TWIN_FONT_TYPE_TTF,\n");
    printf ("\t.name\t\t= \"%s\",\n", face->family_name);
    printf ("\t.style\t\t= \"%s\",\n", face->style_name);
    printf ("\t.n_charmap\t= %d,\n", ncharmap);
    printf ("\t.charmap\t= charmap,\n");
    printf ("\t.outlines\t= outlines,\n");
    printf ("\t.ascender\t= %d,\n", conv (face->ascender, &closure));
    printf ("\t.descender\t= %d,\n", conv (face->descender, &closure));
    printf ("\t.height\t\t= %d,\n", conv (face->height, &closure));
    printf ("};\n");
    return 1;
}

int main (int argc, char **argv)
{
    convert_font (argv[1], 0);
    return 0;
}
