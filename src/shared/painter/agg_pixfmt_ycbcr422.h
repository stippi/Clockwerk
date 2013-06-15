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

#ifndef AGG_PIXFMT_YCBCR422_INCLUDED
#define AGG_PIXFMT_YCBCR422_INCLUDED

#include <string.h>
#include <agg_basics.h>
//#include <agg_color_rgba.h>
#include "agg_color_ycbcr.h"
#include <agg_rendering_buffer.h>

namespace agg
{

    //=====================================================apply_gamma_dir_ycbcr
    template<class ColorT, class GammaLut> class apply_gamma_dir_ycbcr
    {
    public:
        typedef typename ColorT::value_type value_type;

        apply_gamma_dir_ycbcr(const GammaLut& gamma) : m_gamma(gamma) {}

        AGG_INLINE void operator () (value_type* p)
        {
            p[0] = m_gamma.dir(p[0]);
            p[1] = m_gamma.dir(p[1]);
            p[2] = m_gamma.dir(p[2]);
        }

    private:
        const GammaLut& m_gamma;
    };



    //=====================================================apply_gamma_inv_ycbcr
    template<class ColorT, class GammaLut> class apply_gamma_inv_ycbcr
    {
    public:
        typedef typename ColorT::value_type value_type;

        apply_gamma_inv_ycbcr(const GammaLut& gamma) : m_gamma(gamma) {}

        AGG_INLINE void operator () (value_type* p)
        {
            p[0] = m_gamma.inv(p[0]);
            p[1] = m_gamma.inv(p[1]);
            p[2] = m_gamma.inv(p[2]);
        }

    private:
        const GammaLut& m_gamma;
    };


    //=========================================================blender_ycbcr422
    template<class ColorT> struct blender_ycbcr422
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y, unsigned c, 
                                         unsigned alpha, 
                                         unsigned cover=0)
        {
            p[0] += ((y - p[0]) * alpha) >> base_shift;
            p[1] += ((c - p[1]) * alpha) >> base_shift;
        }
    };


    //======================================================blender_ycbcr422_pre
    template<class ColorT> struct blender_ycbcr422_pre
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y, unsigned c,
                                         unsigned alpha,
                                         unsigned cover)
        {
            alpha = color_type::base_mask - alpha;
            cover = (cover + 1) << (base_shift - 8);
            p[0] = (value_type)((p[0] * alpha + y * cover) >> base_shift);
            p[1] = (value_type)((p[1] * alpha + c * cover) >> base_shift);
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y, unsigned c,
                                         unsigned alpha)
        {
            alpha = color_type::base_mask - alpha;
            p[0] = (value_type)(((p[0] * alpha) >> base_shift) + y);
            p[1] = (value_type)(((p[1] * alpha) >> base_shift) + c);
        }

    };



    //===================================================blender_ycbcr422_gamma
    template<class ColorT, class Gamma> class blender_ycbcr422_gamma
    {
    public:
        typedef ColorT color_type;
        typedef Gamma gamma_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        blender_ycbcr422_gamma() : m_gamma(0) {}
        void gamma(const gamma_type& g) { m_gamma = &g; }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pix(value_type* p, 
                                  unsigned y, unsigned c,
                                  unsigned alpha, 
                                  unsigned cover=0)
        {
            calc_type y_src = m_gamma->dir(p[0]);
            calc_type c_src = m_gamma->dir(p[1]);
            p[0] = m_gamma->inv((((m_gamma->dir(y) - y_src) * alpha) >> base_shift) + y_src);
            p[1] = m_gamma->inv((((m_gamma->dir(c) - c_src) * alpha) >> base_shift) + c_src);
        }

    private:
        const gamma_type* m_gamma;
    };



    
    //==================================================pixfmt_alpha_blend_ycbcr422
    template<class Blender, class RenBuf> class pixfmt_alpha_blend_ycbcr422
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
            pix_width  = sizeof(value_type) * 2
        };

    private:
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_even_pix(value_type* p,
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
                }
                else
                {
                    m_blender.blend_pix(p, c.y, c.cb, alpha, cover);
                }
            }
        }
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_odd_pix(value_type* p,
                                              const color_type& c,
                                              unsigned cover)
        {
            if (c.a)
            {
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    p[0] = c.y;
                    p[1] = c.cr;
                }
                else
                {
                    m_blender.blend_pix(p, c.y, c.cr, alpha, cover);
                }
            }
        }
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_2_pix(value_type* p, 
                                            const color_type& c1, 
                                            const color_type& c2, 
                                            unsigned cover1,
                                            unsigned cover2)
        {
        	copy_or_blend_even_pix(p, c1, cover1);
        	copy_or_blend_odd_pix(p + 2, c2, cover2);
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_even_pix(value_type* p, 
                                               const color_type& c)
        {
            if (c.a)
            {
                if(c.a == base_mask)
                {
                    p[0] = c.y;
                    p[1] = c.cb;
                }
                else
                {
                    m_blender.blend_pix(p, c.y, c.cb, c.a);
                }
            }
        }
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_odd_pix(value_type* p, 
                                              const color_type& c)
        {
            if (c.a)
            {
                if(c.a == base_mask)
                {
                    p[0] = c.y;
                    p[1] = c.cr;
                }
                else
                {
                    m_blender.blend_pix(p, c.y, c.cr, c.a);
                }
            }
        }
        //--------------------------------------------------------------------
        AGG_INLINE void copy_or_blend_2_pix(value_type* p, 
                                            const color_type& c1,
                                            const color_type& c2)
        {
        	copy_or_blend_even_pix(p, c1);
        	copy_or_blend_odd_pix(p + 2, c2);
        }


    public:
        //--------------------------------------------------------------------
        pixfmt_alpha_blend_ycbcr422(rbuf_type& rb) :
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
            ((value_type*)p)[0] = c.y;
            ((value_type*)p)[1] = c.cb;
            // TODO: for now, the architecture does not allow us to store
            // all three components. This method is used by an Image
            // Accessor to keep a copy of the "background color" in native
            // pixel format
            //((value_type*)p)[3] = c.cr;
        }

        //--------------------------------------------------------------------
        AGG_INLINE color_type pixel(int x, int y) const
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(y) + x + x;
            int offset = (x & 1) << 1;
            return color_type(p[0], 
                              p[1 - offset], 
                              p[3 - offset]);
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_pixel(int x, int y, const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, 1) + x + x;
            if ((x & 1) == 0)
            {
                p[0] = c.y;
                p[1] = c.cb;
            }
            else
            {
                p[0] = c.y;
                p[1] = c.cr;
            }
        }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            if ((x & 1) == 0)
            {
                copy_or_blend_even_pix((value_type*)m_rbuf->row_ptr(x, y, 1) + x + x, c, cover);
            }
            else
            {
                copy_or_blend_odd_pix((value_type*)m_rbuf->row_ptr(x, y, 1) + x + x, c, cover);
            }
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_hline(int x, int y, 
                                   unsigned len, 
                                   const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x;
            if ((x & 1) == 1)
            {
            	// align loop to start on even pixel
                p[0] = c.y; 
                p[1] = c.cr;
                p += 2;
                len--;
            }
            while(len >= 2)
            {
                p[0] = c.y; 
                p[1] = c.cb; 
                p[2] = c.y; 
                p[3] = c.cr;
                p += 4;
                len -= 2;
            }
            if (len)
            {
                p[0] = c.y; 
                p[1] = c.cb; 
            }
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_vline(int x, int y,
                                   unsigned len, 
                                   const color_type& c)
        {
        	value_type color;
        	if ((x & 1) == 0)
        	{
        		color = c.cb;
        	}
        	else
        	{
        		color = c.cr;
        	}
            do
            {
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
                p[0] = c.y; 
                p[1] = color; 
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
                calc_type alpha = (calc_type(c.a) * (calc_type(cover) + 1)) >> 8;
                if(alpha == base_mask)
                {
		            copy_hline(x, y, len, c);
                }
                else
                {
                    value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x;

		            if ((x & 1) == 1)
		            {
		            	// align loop to start on even pixel
		                m_blender.blend_pix(p, c.y, c.cr, alpha, cover);
		                p += 2;
		                len--;
		            }
		            while(len >= 2)
		            {
		                m_blender.blend_pix(p, c.y, c.cb, alpha, cover);
		                p += 2;
		                m_blender.blend_pix(p, c.y, c.cr, alpha, cover);
		                p += 2;
		                len -= 2;
		            }
		            if (len)
		            {
		                m_blender.blend_pix(p, c.y, c.cb, alpha, cover);
		            }
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
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    copy_vline(x, y, len, c);
                }
                else
                {
		        	value_type color;
		        	if ((x & 1) == 0)
		        	{
		        		color = c.cb;
		        	}
		        	else
		        	{
		        		color = c.cr;
		        	}
		            do
		            {
                        value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;

                        m_blender.blend_pix(p, c.y, color, alpha, cover);
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
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x;

	            if ((x & 1) == 1)
	            {
	            	// align loop to start on even pixel
	            	calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cr;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, c.cr, alpha, *covers);
                    }
	                p += 2;
	                --len;
	                ++covers;
	            }
	            while(len >= 2)
	            {
	            	// even pixel
	            	calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cb;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, c.cb, alpha, *covers);
                    }
                    ++covers;
	                p += 2;
	            	// odd pixel
	            	alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cr;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, c.cr, alpha, *covers);
                    }
                    ++covers;
	                p += 2;
	                len -= 2;
	            }
	            if (len)
	            {
	            	// even pixel
	            	calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cb;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, c.cb, alpha, *covers);
                    }
	            }
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
	        	value_type color;
	        	if ((x & 1) == 0)
	        	{
	        		color = c.cb;
	        	}
	        	else
	        	{
	        		color = c.cr;
	        	}

                do 
                {
                    value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;

                    calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = color;
                    }
                    else
                    {
                        m_blender.blend_pix(p, c.y, color, alpha, *covers);
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
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x;

            if ((x & 1) == 1)
            {
            	// align loop to start on even pixel
                p[0] = colors->y;
                p[1] = colors->cr;
                ++colors;
                
                p += 2;
                len--;
            }
            while(len >= 2)
            {
            	// even pixel
                p[0] = colors->y;
                p[1] = colors->cb;
                ++colors;
                // odd pixel
                p[2] = colors->y;
                p[3] = colors->cr;
                ++colors;

                p += 4;
                len -= 2;
            }
            if (len)
            {
            	// even pixel
                p[0] = colors->y;
                p[1] = colors->cb;
            }
        }


        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + x + x;

            if(covers)
            {
	            if ((x & 1) == 1)
	            {
	            	// align loop to start on even pixel
	            	copy_or_blend_odd_pix(p, *colors++, *covers++);
	            	p += 2;
	            	len--;
	            }
	            while(len >= 2)
                {
                   	// two pixels
	            	copy_or_blend_2_pix(p, colors[0], colors[1], covers[0], covers[1]);
	            	colors += 2;
	            	covers += 2;
	            	p += 4;
	            	len -= 2;
                }
                if (len)
                {
                    // even pixel
	            	copy_or_blend_even_pix(p, *colors, *covers);
                }
            }
            else
            {
                if(cover == 255)
                {
		            if ((x & 1) == 1)
		            {
		            	// align loop to start on even pixel
		            	copy_or_blend_odd_pix(p, *colors++);
		            	p += 2;
		            	len--;
		            }
		            while(len >= 2)
                    {
                    	// two pixels
		            	copy_or_blend_2_pix(p, colors[0], colors[1]);
		            	colors += 2;
		            	p += 4;
		            	len -= 2;
                    }
                    if (len)
                    {
                        // even pixel
                        copy_or_blend_even_pix(p, *colors);
                    }
                }
                else
                {
		            if ((x & 1) == 1)
		            {
		            	// align loop to start on even pixel
		            	copy_or_blend_odd_pix(p, *colors++, cover);
		            	p += 2;
		            	len--;
		            }
		            while(len >= 2)
                    {
                    	// two pixels
                    	copy_or_blend_2_pix(p, colors[0], colors[1], cover, cover);
                    	colors += 2;
		            	p += 4;
		            	len -= 2;
                    }
                    if (len)
                    {
                        // even pixel
                        copy_or_blend_even_pix(p, *colors, cover);
                    }
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
            	if ((x & 1) == 0)
            	{
            		// even column
	                do 
	                {
	                    p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
	
	                    copy_or_blend_even_pix(p, *colors++, *covers++);
	                }
	                while(--len);
            	}
            	else
            	{
            		// odd column
	                do 
	                {
	                    p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
	
	                    copy_or_blend_odd_pix(p, *colors++, *covers++);
	                }
	                while(--len);
            	}
            }
            else
            {
                if(cover == 255)
                {
	            	if ((x & 1) == 0)
	            	{
	            		// even column
	                    do 
	                    {
	                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
	
	                        copy_or_blend_even_pix(p, *colors++);
	                    }
	                    while(--len);
	            	}
	            	else
	            	{
	            		// odd column
	                    do 
	                    {
	                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
	
	                        copy_or_blend_odd_pix(p, *colors++);
	                    }
	                    while(--len);
	            	}
                }
                else
                {
	            	if ((x & 1) == 0)
	            	{
	            		// even column
	                    do 
	                    {
	                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
	
	                        copy_or_blend_even_pix(p, *colors++, cover);
	                    }
	                    while(--len);
	            	}
	            	else
	            	{
	            		// odd column
	                    do 
	                    {
	                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + x + x;
	
	                        copy_or_blend_odd_pix(p, *colors++, cover);
	                    }
	                    while(--len);
	            	}
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
                        p += 2;
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


//        //--------------------------------------------------------------------
//        template<class SrcPixelFormatRenderer>
//        void blend_from(const SrcPixelFormatRenderer& from, 
//                        int xdst, int ydst,
//                        int xsrc, int ysrc,
//                        unsigned len,
//                        int8u cover)
//        {
//            const value_type* psrc = (const value_type*)from.row_ptr(ysrc);
//            if(psrc)
//            {
//                psrc += xsrc * 4;
//                value_type* pdst = 
//                    (value_type*)m_rbuf->row_ptr(xdst, ydst, len) + xdst * 3;   
//
//                if(cover == 255)
//                {
//                    do 
//                    {
//                        value_type alpha = psrc[3];
//                        if(alpha)
//                        {
//                            if(alpha == base_mask)
//                            {
//                                pdst[0] = psrc[0];
//                                pdst[1] = psrc[1];
//                                pdst[2] = psrc[2];
//                            }
//                            else
//                            {
//                                m_blender.blend_pix(pdst, 
//                                                    psrc[0],
//                                                    psrc[1],
//                                                    psrc[2],
//                                                    alpha);
//                            }
//                        }
//                        psrc += 4;
//                        pdst += 3;
//                    }
//                    while(--len);
//                }
//                else
//                {
//                    color_type color;
//                    do 
//                    {
//                        color.r = psrc[0];
//                        color.g = psrc[1];
//                        color.b = psrc[2];
//                        color.a = psrc[3];
//                        copy_or_blend_pix(pdst, color, cover);
//                        psrc += 4;
//                        pdst += 3;
//                    }
//                    while(--len);
//                }
//            }
//        }

    private:
        rbuf_type* m_rbuf;
        Blender    m_blender;
    };

    typedef pixfmt_alpha_blend_ycbcr422<blender_ycbcr422<ycbcra8>, rendering_buffer> pixfmt_ycbcr422;    //----pixfmt_ycbcr422

    typedef pixfmt_alpha_blend_ycbcr422<blender_ycbcr422_pre<ycbcra8>, rendering_buffer> pixfmt_ycbcr422_pre; //----pixfmt_ycbcr422_pre

//    //-----------------------------------------------------pixfmt_ycbcr16_gamma
//    template<class Gamma> class pixfmt_ycbcr16_gamma : 
//    public pixfmt_alpha_blend_ycbcr422<blender_ycbcr422_gamma<rgba16, Gamma>, rendering_buffer>
//    {
//    public:
//        pixfmt_rgb24_gamma(rendering_buffer& rb, const Gamma& g) :
//            pixfmt_alpha_blend_ycbcr422<blender_ycbcr422_gamma<ycbcra16, Gamma>, rendering_buffer>(rb) 
//        {
//            this->blender().gamma(g);
//        }
//    };
        

}

#endif

