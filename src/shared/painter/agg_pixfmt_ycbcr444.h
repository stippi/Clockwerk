/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: superstippi@gmx.de
//----------------------------------------------------------------------------

#ifndef AGG_PIXFMT_YCBCR444_INCLUDED
#define AGG_PIXFMT_YCBCR444_INCLUDED

#include <string.h>
#include <agg_basics.h>
#include "agg_color_ycbcr.h"
#include "agg_pixfmt_ycbcr422.h"	// for gamma (TODO: remove gamma stuff?)
#include <agg_rendering_buffer.h>

namespace agg
{

    //=========================================================blender_ycbcr444
    template<class ColorT> struct blender_ycbcr444
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y,
                                         unsigned cb,
                                         unsigned cr,
                                         unsigned alpha, 
                                         unsigned cover=0)
        {
            p[0] += ((y - p[0]) * alpha) >> base_shift;
            p[1] += ((cb - p[1]) * alpha) >> base_shift;
            p[2] += ((cr - p[2]) * alpha) >> base_shift;
        }
    };


    //======================================================blender_ycbcr444_pre
    template<class ColorT> struct blender_ycbcr444_pre
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y,
                                         unsigned cb,
                                         unsigned cr,
                                         unsigned alpha,
                                         unsigned cover)
        {
            alpha = color_type::base_mask - alpha;
            cover = (cover + 1) << (base_shift - 8);
            p[0] = (value_type)((p[0] * alpha + y * cover) >> base_shift);
            p[1] = (value_type)((p[1] * alpha + cb * cover) >> base_shift);
            p[2] = (value_type)((p[2] * alpha + cr * cover) >> base_shift);
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y,
                                         unsigned cb,
                                         unsigned cr,
                                         unsigned alpha)
        {
            alpha = color_type::base_mask - alpha;
            p[0] = (value_type)(((p[0] * alpha) >> base_shift) + y);
            p[1] = (value_type)(((p[1] * alpha) >> base_shift) + cb);
            p[2] = (value_type)(((p[2] * alpha) >> base_shift) + cr);
        }

    };



    //===================================================blender_ycbcr444_gamma
    template<class ColorT, class Gamma> class blender_ycbcr444_gamma
    {
    public:
        typedef ColorT color_type;
        typedef Gamma gamma_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        blender_ycbcr444_gamma() : m_gamma(0) {}
        void gamma(const gamma_type& g) { m_gamma = &g; }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pix(value_type* p, 
                                  unsigned y,
                                  unsigned cb,
                                  unsigned cr,
                                  unsigned alpha, 
                                  unsigned cover=0)
        {
            calc_type y_src = m_gamma->dir(p[0]);
            calc_type cb_src = m_gamma->dir(p[1]);
            calc_type cr_src = m_gamma->dir(p[2]);
            p[0] = m_gamma->inv((((m_gamma->dir(y) - y_src) * alpha) >> base_shift) + y_src);
            p[1] = m_gamma->inv((((m_gamma->dir(cb) - cb_src) * alpha) >> base_shift) + cb_src);
            p[2] = m_gamma->inv((((m_gamma->dir(cr) - cr_src) * alpha) >> base_shift) + cr_src);
        }

    private:
        const gamma_type* m_gamma;
    };



    
    //==================================================pixfmt_alpha_blend_ycbcr444
    template<class Blender, class RenBuf> class pixfmt_alpha_blend_ycbcr444
    {
    public:
        typedef RenBuf   rbuf_type;
        typedef typename rbuf_type::row_data row_data;
        typedef Blender  blender_type;
        typedef typename blender_type::color_type color_type;
        typedef int                               order_type; // A fake one
        typedef typename color_type::value_type   value_type;
        typedef typename color_type::calc_type    calc_type;
        enum base_scale_e 
        {
            base_shift = color_type::base_shift,
            base_scale = color_type::base_scale,
            base_mask  = color_type::base_mask,
            pix_width  = sizeof(value_type) * 3
        };

    private:
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_pix(value_type* p,
                                          const color_type& c,
                                          unsigned cover)
        {
            if (c.a)
            {
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    p[0] = c.y;
                    p[1] = c.cb;
                    p[2] = c.cr;
                }
                else
                {
                    m_blender.blend_pix(p, c.y, c.cb, c.cr, alpha, cover);
                }
            }
        }
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_pix(value_type* p, 
                                          const color_type& c)
        {
            if (c.a)
            {
                if(c.a == base_mask)
                {
                    p[0] = c.y;
                    p[1] = c.cb;
                    p[2] = c.cr;
                }
                else
                {
                    m_blender.blend_pix(p, c.y, c.cb, c.cr, c.a);
                }
            }
        }


    public:
        //--------------------------------------------------------------------
        pixfmt_alpha_blend_ycbcr444(rbuf_type& rb) :
            m_rbuf(&rb)
        {}

        //--------------------------------------------------------------------
        Blender& blender() { return m_blender; }

        //--------------------------------------------------------------------
        AGG_INLINE unsigned width()  const { return m_rbuf->width();  }
        AGG_INLINE unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        const int8u* row_ptr(int y) const
        {
            return m_rbuf->row_ptr(y);
        }

        //--------------------------------------------------------------------
        const int8u* pix_ptr(int x, int y) const
        {
            return m_rbuf->row_ptr(y) + x * pix_width;
        }

        //--------------------------------------------------------------------
        row_data row(int x, int y) const
        {
            return m_rbuf->row(y);
        }

        //--------------------------------------------------------------------
        AGG_INLINE static void make_pix(int8u* p, const color_type& c)
        {
        	// fills the buffer given in p, which should be pixel_width
        	// bytes wide, with the color given in c
			// This method is used by an Image Accessor to keep a copy of
			// the "background color" in native pixel format
            ((value_type*)p)[0] = c.y;
            ((value_type*)p)[1] = c.cb;
            ((value_type*)p)[2] = c.cr;
        }

        //--------------------------------------------------------------------
        AGG_INLINE color_type pixel(int x, int y) const
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(y) + x + x + x;
            return color_type(p[0], p[1], p[2]);
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_pixel(int x, int y, const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, 1) + x + x + x;
            p[0] = c.y;
            p[1] = c.cb;
            p[1] = c.cr;
        }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            copy_or_blend_pix((value_type*)m_rbuf->row_ptr(x, y, 1) + x + x + x, c, cover);
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_hline(int x, int y, 
                                   unsigned len, 
                                   const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x + x;
            do
            {
                p[0] = c.y; 
                p[1] = c.cb; 
                p[2] = c.cr;
                p += 3;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_vline(int x, int y,
                                   unsigned len, 
                                   const color_type& c)
        {
            do
            {
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x + x;
                p[0] = c.y; 
                p[1] = c.cb; 
                p[2] = c.cr;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_hline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            if (c.a)
            {
                value_type* p = (value_type*)
                	m_rbuf->row_ptr(x, y, len) + x + x + x;

                calc_type alpha = (calc_type(c.a) * (calc_type(cover) + 1)) >> 8;
                if(alpha == base_mask)
                {
                    do
                    {
                        p[0] = c.y; 
                        p[1] = c.cb; 
                        p[2] = c.cr;
                        p += 3;
                    }
                    while(--len);
                }
                else
                {
                    do
                    {
                        m_blender.blend_pix(p, c.y, c.cb, c.cr, alpha, cover);
                        p += 3;
                    }
                    while(--len);
                }
            }
        }


        //--------------------------------------------------------------------
        void blend_vline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            if (c.a)
            {
                value_type* p;
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    do
                    {
                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x + x;

                        p[0] = c.y;
                        p[1] = c.cb;
                        p[2] = c.cr;
                    }
                    while(--len);
                }
                else
                {
                    do
                    {
                        p = (value_type*)
                            m_rbuf->row_ptr(x, y++, 1) + x + x + x;

                        m_blender.blend_pix(p, c.y, c.cb, c.cr, alpha, cover);
                    }
                    while(--len);
                }
            }
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            if (c.a)
            {
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x + x;

                do 
                {
                    calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cb;
                        p[2] = c.cr;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, c.cb, c.cr, alpha, *covers);
                    }
                    p += 3;
                    ++covers;
                }
                while(--len);
            }
        }


        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            if (c.a)
            {
                do 
                {
                    value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x + x;

                    calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cb;
                        p[2] = c.cr;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, c.cb, c.cr, alpha, *covers);
                    }
                    ++covers;
                }
                while(--len);
            }
        }


        //--------------------------------------------------------------------
        void copy_color_hspan(int x, int y,
                              unsigned len, 
                              const color_type* colors)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x + x;

            do 
            {
                p[0] = colors->y;
                p[1] = colors->cb;
                p[2] = colors->cr;
                ++colors;
                p += 3;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x + x;

            if(covers)
            {
                do 
                {
                    copy_or_blend_pix(p, *colors++, *covers++);
                    p += 3;
                }
                while(--len);
            }
            else
            {
                if(cover == 255)
                {
                    do 
                    {
                        copy_or_blend_pix(p, *colors++);
                        p += 3;
                    }
                    while(--len);
                }
                else
                {
                    do 
                    {
                        copy_or_blend_pix(p, *colors++, cover);
                        p += 3;
                    }
                    while(--len);
                }
            }
        }



        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p;
            if(covers)
            {
                do 
                {
                    p = (value_type*)
                        m_rbuf->row_ptr(x, y++, 1) + x + x + x;

                    copy_or_blend_pix(p, *colors++, *covers++);
                }
                while(--len);
            }
            else
            {
                if(cover == 255)
                {
                    do 
                    {
                        p = (value_type*)
                            m_rbuf->row_ptr(x, y++, 1) + x + x + x;

                        copy_or_blend_pix(p, *colors++);
                    }
                    while(--len);
                }
                else
                {
                    do 
                    {
                        p = (value_type*)
                            m_rbuf->row_ptr(x, y++, 1) + x + x + x;

                        copy_or_blend_pix(p, *colors++, cover);
                    }
                    while(--len);
                }
            }
        }

        //--------------------------------------------------------------------
        template<class Function> void for_each_pixel(Function f)
        {
            unsigned y;
            for(y = 0; y < height(); ++y)
            {
                row_data r = m_rbuf->row(y);
                if(r.ptr)
                {
                    unsigned len = r.x2 - r.x1 + 1;
                    value_type* p = (value_type*)
                        m_rbuf->row_ptr(r.x1, y, len) + r.x1 * pix_width;
                    do
                    {
                        f(p);
                        p += 3;
                    }
                    while(--len);
                }
            }
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_dir(const GammaLut& g)
        {
            for_each_pixel(apply_gamma_dir_ycbcr<color_type, GammaLut>(g));
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_inv(const GammaLut& g)
        {
            for_each_pixel(apply_gamma_inv_ycbcr<color_type, GammaLut>(g));
        }

        //--------------------------------------------------------------------
        template<class RenBuf2>
        void copy_from(const RenBuf2& from, 
                       int xdst, int ydst,
                       int xsrc, int ysrc,
                       unsigned len)
        {
            const int8u* p = from.row_ptr(ysrc);
            if(p)
            {
                memmove(m_rbuf->row_ptr(xdst, ydst, len) + xdst * pix_width, 
                        p + xsrc * pix_width, 
                        len * pix_width);
            }
        }


        //--------------------------------------------------------------------
        template<class SrcPixelFormatRenderer>
        void blend_from(const SrcPixelFormatRenderer& from, 
                        int xdst, int ydst,
                        int xsrc, int ysrc,
                        unsigned len,
                        int8u cover)
        {
            const value_type* psrc = (const value_type*)from.row_ptr(ysrc);
            if(psrc)
            {
                psrc += xsrc * 4;
                value_type* pdst = 
                    (value_type*)m_rbuf->row_ptr(xdst, ydst, len) + xdst * 3;   

                if(cover == 255)
                {
                    do 
                    {
                        value_type alpha = psrc[3];
                        if(alpha)
                        {
                            if(alpha == base_mask)
                            {
                                pdst[0] = psrc[0];
                                pdst[1] = psrc[1];
                                pdst[2] = psrc[2];
                            }
                            else
                            {
                                m_blender.blend_pix(pdst, 
                                                    psrc[0],
                                                    psrc[1],
                                                    psrc[2],
                                                    alpha);
                            }
                        }
                        psrc += 4;
                        pdst += 3;
                    }
                    while(--len);
                }
                else
                {
                    color_type color;
                    do 
                    {
                        color.y = psrc[0];
                        color.cb = psrc[1];
                        color.cr = psrc[2];
                        color.a = psrc[3];
                        copy_or_blend_pix(pdst, color, cover);
                        psrc += 4;
                        pdst += 3;
                    }
                    while(--len);
                }
            }
        }

    private:
        rbuf_type* m_rbuf;
        Blender    m_blender;
    };

    typedef pixfmt_alpha_blend_ycbcr444<blender_ycbcr444<ycbcra8>, rendering_buffer> pixfmt_ycbcr444;    //----pixfmt_ycbcr444

    typedef pixfmt_alpha_blend_ycbcr444<blender_ycbcr444_pre<ycbcra8>, rendering_buffer> pixfmt_ycbcr444_pre; //----pixfmt_ycbcr444_pre

//    //-----------------------------------------------------pixfmt_ycbcr16_gamma
//    template<class Gamma> class pixfmt_ycbcr16_gamma : 
//    public pixfmt_alpha_blend_ycbcr444<blender_ycbcr444_gamma<rgba16, Gamma>, rendering_buffer>
//    {
//    public:
//        pixfmt_rgb24_gamma(rendering_buffer& rb, const Gamma& g) :
//            pixfmt_alpha_blend_ycbcr444<blender_ycbcr444_gamma<ycbcra16, Gamma>, rendering_buffer>(rb) 
//        {
//            this->blender().gamma(g);
//        }
//    };
        

}

#endif

