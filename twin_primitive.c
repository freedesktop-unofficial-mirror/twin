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

static twin_argb32_t __inline
in_over (twin_argb32_t	dst,
	 twin_argb32_t	src,
	 twin_a8_t	msk)
{
    uint16_t	t1, t2, t3, t4;
    twin_a8_t	a;

    src = (twin_in(src,0,msk,t1) |
	   twin_in(src,8,msk,t2) |
	   twin_in(src,16,msk,t3) |
	   twin_in(src,24,msk,t4));
    a = ~(src >> 24);
    return (twin_over (src, dst, 0, a, t1) |
	    twin_over (src, dst, 8, a, t2) |
	    twin_over (src, dst, 16, a, t3) |
	    twin_over (src, dst, 24, a, t4));
}
    
static twin_argb32_t __inline
in (twin_argb32_t   src,
    twin_a8_t	    msk)
{
    uint16_t	t1, t2, t3, t4;

    return (twin_in(src,0,msk,t1) |
	    twin_in(src,8,msk,t2) |
	    twin_in(src,16,msk,t3) |
	    twin_in(src,24,msk,t4));
}
    
static twin_argb32_t __inline
over (twin_argb32_t	dst,
      twin_argb32_t	src)
{
    uint16_t	t1, t2, t3, t4;
    twin_a8_t	a;

    a = ~(src >> 24);
    return (twin_over (src, dst, 0, a, t1) |
	    twin_over (src, dst, 8, a, t2) |
	    twin_over (src, dst, 16, a, t3) |
	    twin_over (src, dst, 24, a, t4));
}

static twin_argb32_t __inline
rgb16_to_argb32 (twin_rgb16_t v)
{
    return twin_rgb16_to_argb32(v);
}

static twin_argb32_t __inline
a8_to_argb32 (twin_a8_t v)
{
    return v << 24;
}

static twin_rgb16_t __inline
argb32_to_rgb16 (twin_argb32_t v)
{
    return twin_argb32_to_rgb16 (v);
}

static twin_a8_t __inline
argb32_to_a8 (twin_argb32_t v)
{
    return v >> 24;
}

/*
 * Naming convention
 *
 *  _twin_<src>_in_<msk>_op_<dst>
 *
 * Use 'c' for constant
 */

#define dst_argb32_get	    (*dst.argb32)
#define dst_argb32_set	    (*dst.argb32++) = 
#define dst_rgb16_get	    (rgb16_to_argb32(*dst.rgb16))
#define dst_rgb16_set	    (*dst.rgb16++) = argb32_to_rgb16
#define dst_a8_get	    (a8_to_argb32(*dst.a8))
#define dst_a8_set	    (*dst.a8++) = argb32_to_a8

#define src_c		    (src.c)
#define src_argb32	    (*src.p.argb32++)
#define src_rgb16	    (rgb16_to_argb32(*src.p.rgb16++))
#define src_a8		    (a8_to_argb32(*src.p.a8++))

#define msk_c		    (argb32_to_a8 (msk.c))
#define msk_argb32	    (argb32_to_a8 (*msk.p.argb32++))
#define msk_rgb16	    (0xff)
#define msk_a8		    (*msk.p.a8++)

#define cat2(a,b) a##b
#define cat3(a,b,c) a##b##c
#define cat4(a,b,c,d) a##b##c##d
#define cat6(a,b,c,d,e,f) a##b##c##d##e##f

#define _twin_in_op_name(src,op,msk,dst) cat6(_twin_,src,_in_,msk,op,dst)

#define _twin_op_name(src,op,dst) cat4(_twin_,src,op,dst)

#define make_twin_in_over(__dst,__src,__msk) \
void \
_twin_in_op_name(__src,_over_,__msk,__dst)(twin_pointer_t   dst, \
					   twin_source_u    src, \
					   twin_source_u    msk, \
					   int		    width) \
{ \
    twin_argb32_t   dst32; \
    twin_argb32_t   src32; \
    twin_a8_t	    msk8; \
    while (width--) { \
	dst32 = cat3(dst_,__dst,_get); \
	src32 = cat2(src_,__src); \
	msk8 = cat2(msk_,__msk); \
	dst32 = in_over (dst32, src32, msk8); \
	cat3(dst_,__dst,_set) (dst32); \
    } \
}

#define make_twin_in_source(__dst,__src,__msk) \
void \
_twin_in_op_name(__src,_source_,__msk,__dst)(twin_pointer_t	dst, \
					     twin_source_u	src, \
					     twin_source_u	msk, \
					     int		width) \
{ \
    twin_argb32_t   dst32; \
    twin_argb32_t   src32; \
    twin_a8_t	    msk8; \
    while (width--) { \
	src32 = cat2(src_,__src); \
	msk8 = cat2(msk_,__msk); \
	dst32 = in (src32, msk8); \
	cat3(dst_,__dst,_set) (dst32); \
    } \
}

#define make_twin_in_op_msks(op,dst,src) \
cat2(make_twin_in_,op)(dst,src,argb32) \
cat2(make_twin_in_,op)(dst,src,rgb16) \
cat2(make_twin_in_,op)(dst,src,a8) \
cat2(make_twin_in_,op)(dst,src,c)

#define make_twin_in_op_srcs_msks(op,dst) \
make_twin_in_op_msks(op,dst,argb32) \
make_twin_in_op_msks(op,dst,rgb16) \
make_twin_in_op_msks(op,dst,a8) \
make_twin_in_op_msks(op,dst,c)

#define make_twin_in_op_dsts_srcs_msks(op) \
make_twin_in_op_srcs_msks(op,argb32) \
make_twin_in_op_srcs_msks(op,rgb16) \
make_twin_in_op_srcs_msks(op,a8)

make_twin_in_op_dsts_srcs_msks(over)
make_twin_in_op_dsts_srcs_msks(source)

#define make_twin_over(__dst,__src) \
void \
_twin_op_name(__src,_over_,__dst) (twin_pointer_t   dst, \
				   twin_source_u    src, \
				   int		    width) \
{ \
    twin_argb32_t   dst32; \
    twin_argb32_t   src32; \
    while (width--) { \
	dst32 = cat3(dst_,__dst,_get); \
	src32 = cat2(src_,__src); \
	dst32 = over (dst32, src32); \
	cat3(dst_,__dst,_set) (dst32); \
    } \
}

#define make_twin_source(__dst,__src) \
void \
_twin_op_name(__src,_source_,__dst) (twin_pointer_t dst, \
				     twin_source_u  src, \
				     int	    width) \
{ \
    twin_argb32_t   dst32; \
    twin_argb32_t   src32; \
    while (width--) { \
	src32 = cat2(src_,__src); \
	dst32 = src32; \
	cat3(dst_,__dst,_set) (dst32); \
    } \
}

#define make_twin_op_srcs(op,dst) \
cat2(make_twin_,op)(dst,argb32) \
cat2(make_twin_,op)(dst,rgb16) \
cat2(make_twin_,op)(dst,a8) \
cat2(make_twin_,op)(dst,c)

#define make_twin_op_dsts_srcs(op) \
make_twin_op_srcs(op,argb32) \
make_twin_op_srcs(op,rgb16) \
make_twin_op_srcs(op,a8)

make_twin_op_dsts_srcs(over)
make_twin_op_dsts_srcs(source)