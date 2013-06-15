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
//
// color types ycbcra8
//
//----------------------------------------------------------------------------

#ifndef AGG_COLOR_YCBCR_INCLUDED
#define AGG_COLOR_YCBCR_INCLUDED

#include <agg_basics.h>
#include <agg_color_rgba.h>


namespace agg
{

    //===================================================================ycbcra8
    struct ycbcra8
    {
        typedef int8u  value_type;
        typedef int32u calc_type;
        typedef int32  long_type;
        enum base_scale_e
        {
            base_shift = 8,
            base_scale = 1 << base_shift,
            base_mask  = base_scale - 1
        };
        typedef ycbcra8 self_type;

        value_type y;
        value_type cb;
        value_type cr;
        value_type a;


//		// ycbcr_to_rgb
//		inline void
//		ycbcr_to_rgb(value_type y, value_type cb, value_type cr,
//					 value_type& r, value_type& g, value_type& b)
//		{
//			// TODO: optimize for fixed point...
//			r = (value_type)max_c(0, min_c(255, 1.164 * (y - 16) + 1.596 * (cr - 128)));
//			g = (value_type)max_c(0, min_c(255, 1.164 * (y - 16) - 0.813 * (cr - 128) - 0.391 * (cb - 128)));
//			b = (value_type)max_c(0, min_c(255, 1.164 * (y - 16) + 2.018 * (cb - 128)));
//		}
	
		// rgb_to_ycbcr
		inline void
		rgb_to_ycbcr(value_type r, value_type g, value_type b,
					 value_type& y, value_type& cb, value_type& cr)
		{
			y = (8432 * r + 16425 * g + 3176 * b) / 32768 + 16;
			cb = (-4818 * r - 9527 * g + 14345 * b) / 32768 + 128;
			cr = (14345 * r - 12045 * g - 2300 * b) / 32768 + 128;
		}


        //--------------------------------------------------------------------
        ycbcra8() {}

        //--------------------------------------------------------------------
        ycbcra8(unsigned y_, unsigned cb_, unsigned cr_, unsigned a_=base_mask) :
            y(value_type(y_)), cb(value_type(cb_)), cr(value_type(cr_)), a(value_type(a_)) {}

        //--------------------------------------------------------------------
        ycbcra8(const self_type& c, unsigned a_) :
            y(c.y), cb(c.cb), cr(c.cr), a(value_type(a_)) {}

        //--------------------------------------------------------------------
        ycbcra8(const rgba& c) :
            a((value_type)uround(c.a * double(base_mask)))
        {
        	value_type r = (value_type)(c.r * double(base_mask));
        	value_type g = (value_type)(c.g * double(base_mask));
        	value_type b = (value_type)(c.b * double(base_mask));
            rgb_to_ycbcr(r, g, b, y, cb, cr);
        }

        //--------------------------------------------------------------------
        ycbcra8(const rgba& c, double a_) :
            a((value_type)uround(a_ * double(base_mask)))
        {
        	value_type r = (value_type)(c.r * double(base_mask));
        	value_type g = (value_type)(c.g * double(base_mask));
        	value_type b = (value_type)(c.b * double(base_mask));
            rgb_to_ycbcr(r, g, b, y, cb, cr);
        }

        //--------------------------------------------------------------------
        ycbcra8(const rgba8& c) :
            a(c.a)
        {
            rgb_to_ycbcr(c.r, c.g, c.b, y, cb, cr);
        }

        //--------------------------------------------------------------------
        ycbcra8(const rgba8& c, unsigned a_) :
            a(a_)
        {
            rgb_to_ycbcr(c.r, c.g, c.b, y, cb, cr);
        }

        //--------------------------------------------------------------------
        void clear()
        {
            y = 16;
            cb = 128;
            cr = 128;
            a = 0;
        }

        //--------------------------------------------------------------------
        const self_type& transparent()
        {
            a = 0;
            return *this;
        }

        //--------------------------------------------------------------------
        void opacity(double a_)
        {
            if(a_ < 0.0) a_ = 0.0;
            if(a_ > 1.0) a_ = 1.0;
            a = (value_type)uround(a_ * double(base_mask));
        }

        //--------------------------------------------------------------------
        double opacity() const
        {
            return double(a) / double(base_mask);
        }


        //--------------------------------------------------------------------
        const self_type& premultiply()
        {
            if(a == base_mask) return *this;
            if(a == 0)
            {
                y = 0;
                cb = 0;
                cr = 0;
                return *this;
            }
            y = value_type((calc_type(y) * a) >> base_shift);
            cb = value_type((calc_type(cb) * a) >> base_shift);
            cr = value_type((calc_type(cr) * a) >> base_shift);
            return *this;
        }

        //--------------------------------------------------------------------
        const self_type& premultiply(unsigned a_)
        {
            if(a == base_mask && a_ >= base_mask) return *this;
            if(a == 0 || a_ == 0)
            {
                y = 0;
                cb = 0;
                cr = 0;
                a = 0;
                return *this;
            }
            calc_type y_ = (calc_type(y) * a_) / a;
            y = value_type((y_ > a_) ? a_ : y_);

            calc_type cb_ = (calc_type(cb) * a_) / a;
            cb = value_type((cb_ > a_) ? a_ : cb_);

            calc_type cr_ = (calc_type(cr) * a_) / a;
            cr = value_type((cr_ > a_) ? a_ : cr_);

            a = value_type(a_);
            return *this;
        }

        //--------------------------------------------------------------------
        const self_type& demultiply()
        {
            if(a == base_mask) return *this;
            if(a == 0)
            {
                y = 0;
                cb = 0;
                cr = 0;
                return *this;
            }
            calc_type y_ = (calc_type(y) * base_mask) / a;
            y = value_type((y_ > base_mask) ? base_mask : y_);
            calc_type cb_ = (calc_type(cb) * base_mask) / a;
            cb = value_type((cb_ > base_mask) ? base_mask : cb_);
            calc_type cr_ = (calc_type(cr) * base_mask) / a;
            cr = value_type((cr_ > base_mask) ? base_mask : cr_);
            return *this;
        }

        //--------------------------------------------------------------------
        self_type gradient(self_type c, double k) const
        {
            self_type ret;
            calc_type ik = uround(k * base_scale);
            ret.y = value_type(calc_type(y) + (((calc_type(c.y) - y) * ik) >> base_shift));
            ret.cb = value_type(calc_type(cb) + (((calc_type(c.cb) - cb) * ik) >> base_shift));
            ret.cr = value_type(calc_type(cr) + (((calc_type(c.cr) - cr) * ik) >> base_shift));
            ret.a = value_type(calc_type(a) + (((calc_type(c.a) - a) * ik) >> base_shift));
            return ret;
        }

        //--------------------------------------------------------------------
        static self_type no_color() { return self_type(16, 128, 128, 0); }
    };


    //-------------------------------------------------------------ycbcra8_pre
    inline ycbcra8 ycbcra8_pre(unsigned y, unsigned cb, unsigned cr, unsigned a = ycbcra8::base_mask)
    {
        return ycbcra8(y,cb,cr,a).premultiply();
    }
    inline ycbcra8 ycbcra8_pre(const ycbcra8& c, unsigned a)
    {
        return ycbcra8(c,a).premultiply();
    }
    inline ycbcra8 ycbcra8_pre(const rgba& c)
    {
        return ycbcra8(c).premultiply();
    }
    inline ycbcra8 ycbcra8_pre(const rgba& c, double a)
    {
        return ycbcra8(c,a).premultiply();
    }
    inline ycbcra8 ycbcra8_pre(const rgba8& c)
    {
        return ycbcra8(c).premultiply();
    }
    inline ycbcra8 ycbcra8_pre(const rgba8& c, unsigned a)
    {
        return ycbcra8(c,a).premultiply();
    }



}




#endif
