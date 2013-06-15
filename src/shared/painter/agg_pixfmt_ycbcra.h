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
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Adaptation for high precision colors has been sponsored by 
// Liberty Technology Systems, Inc., visit http://lib-sys.com
//
// Liberty Technology Systems, Inc. is the provider of
// PostScript and PDF technology for software developers.
// 
//----------------------------------------------------------------------------

#ifndef AGG_PIXFMT_YCBCRA_INCLUDED
#define AGG_PIXFMT_YCBCRA_INCLUDED

#include <string.h>
#include <math.h>

#include <agg_basics.h>
#include <agg_pixfmt_rgba.h>	// for min/max

#include "agg_color_ycbcr.h"
#include "agg_pixfmt_ycbcr422.h"	// for gamma (TODO: remove gamma stuff?)
#include "agg_rendering_buffer.h"

namespace agg
{

    //=========================================================multiplier_ycbcra
    template<class ColorT> struct multiplier_ycbcra
    {
        typedef typename ColorT::value_type value_type;
        typedef typename ColorT::calc_type calc_type;

        //--------------------------------------------------------------------
        static AGG_INLINE void premultiply(value_type* p)
        {
            calc_type a = p[3];
            if(a < ColorT::base_mask)
            {
                if(a == 0)
                {
                    p[0] = p[1] = p[2] = 0;
                    return;
                }
                p[0] = value_type((p[0] * a + ColorT::base_mask) >> ColorT::base_shift);
                p[1] = value_type((p[1] * a + ColorT::base_mask) >> ColorT::base_shift);
                p[2] = value_type((p[2] * a + ColorT::base_mask) >> ColorT::base_shift);
            }
        }


        //--------------------------------------------------------------------
        static AGG_INLINE void demultiply(value_type* p)
        {
            calc_type a = p[3];
            if(a < ColorT::base_mask)
            {
                if(a == 0)
                {
                    p[0] = p[1] = p[2] = 0;
                    return;
                }
                calc_type y = (calc_type(p[0]) * ColorT::base_mask) / a;
                calc_type cb = (calc_type(p[1]) * ColorT::base_mask) / a;
                calc_type cr = (calc_type(p[2]) * ColorT::base_mask) / a;
                p[0] = value_type((y > ColorT::base_mask) ? ColorT::base_mask : y);
                p[1] = value_type((cb > ColorT::base_mask) ? ColorT::base_mask : cb);
                p[2] = value_type((cr > ColorT::base_mask) ? ColorT::base_mask : cr);
            }
        }
    };



    







    //=============================================================blender_ycbcra
    template<class ColorT> struct blender_ycbcra
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e 
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned alpha, 
                                         unsigned cover=0)
        {
            calc_type y = p[0];
            calc_type cb = p[1];
            calc_type cr = p[2];
            calc_type a = p[3];
            p[0] = (value_type)(((cy - y) * alpha + (y << base_shift)) >> base_shift);
            p[1] = (value_type)(((ccb - cb) * alpha + (cb << base_shift)) >> base_shift);
            p[2] = (value_type)(((ccr - cr) * alpha + (cr << base_shift)) >> base_shift);
            p[3] = (value_type)((alpha + a) - ((alpha * a + base_mask) >> base_shift));
        }
    };

    //=========================================================blender_ycbcra_pre
    template<class ColorT> struct blender_ycbcra_pre
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned alpha,
                                         unsigned cover)
        {
            alpha = color_type::base_mask - alpha;
            cover = (cover + 1) << (base_shift - 8);
            p[0] = (value_type)((p[0] * alpha + cy * cover) >> base_shift);
            p[1] = (value_type)((p[1] * alpha + ccb * cover) >> base_shift);
            p[2] = (value_type)((p[2] * alpha + ccr * cover) >> base_shift);
            p[3] = (value_type)(base_mask - ((alpha * (base_mask - p[3])) >> base_shift));
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned alpha)
        {
            alpha = color_type::base_mask - alpha;
            p[0] = (value_type)(((p[0] * alpha) >> base_shift) + cy);
            p[1] = (value_type)(((p[1] * alpha) >> base_shift) + ccb);
            p[2] = (value_type)(((p[2] * alpha) >> base_shift) + ccr);
            p[3] = (value_type)(base_mask - ((alpha * (base_mask - p[3])) >> base_shift));
        }
    };

    //======================================================blender_ycbcra_plain
    template<class ColorT> struct blender_ycbcra_plain
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e { base_shift = color_type::base_shift };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned alpha,
                                         unsigned cover=0)
        {
            if(alpha == 0) return;
            calc_type a = p[3];
            calc_type y = p[0] * a;
            calc_type cb = p[1] * a;
            calc_type cr = p[2] * a;
            a = ((alpha + a) << base_shift) - alpha * a;
            p[3] = (value_type)(a >> base_shift);
            p[0] = (value_type)((((cy << base_shift) - y) * alpha + (y << base_shift)) / a);
            p[1] = (value_type)((((ccb << base_shift) - cb) * alpha + (cb << base_shift)) / a);
            p[2] = (value_type)((((ccr << base_shift) - cr) * alpha + (cr << base_shift)) / a);
        }
    };











    //=========================================================comp_op_ycbcra_clear
    template<class ColorT> struct comp_op_ycbcra_clear
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned, unsigned, unsigned, unsigned,
                                         unsigned cover)
        {
            if(cover < 255)
            {
                cover = 255 - cover;
                p[0] = (value_type)((p[0] * cover + 255) >> 8);
                p[1] = (value_type)((p[1] * cover + 255) >> 8);
                p[2] = (value_type)((p[2] * cover + 255) >> 8);
                p[3] = (value_type)((p[3] * cover + 255) >> 8);
            }
            else
            {
                p[0] = p[1] = p[2] = p[3] = 0;
            }
        }
    };

    //===========================================================comp_op_ycbcra_src
    template<class ColorT> struct comp_op_ycbcra_src
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;

        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                unsigned alpha = 255 - cover;
                p[0] = (value_type)(((p[0] * alpha + 255) >> 8) + ((sr * cover + 255) >> 8));
                p[1] = (value_type)(((p[1] * alpha + 255) >> 8) + ((sg * cover + 255) >> 8));
                p[2] = (value_type)(((p[2] * alpha + 255) >> 8) + ((sb * cover + 255) >> 8));
                p[3] = (value_type)(((p[3] * alpha + 255) >> 8) + ((sa * cover + 255) >> 8));
            }
            else
            {
                p[0] = sr;
                p[1] = sg;
                p[2] = sb;
                p[3] = sa;
            }
        }
    };

    //===========================================================comp_op_ycbcra_dst
    template<class ColorT> struct comp_op_ycbcra_dst
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;

        static AGG_INLINE void blend_pix(value_type*, 
                                         unsigned, unsigned, unsigned, 
                                         unsigned, unsigned)
        {
        }
    };

    //======================================================comp_op_ycbcra_src_over
    template<class ColorT> struct comp_op_ycbcra_src_over
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        //   Dca' = Sca + Dca.(1 - Sa)
        //   Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type s1a = base_mask - sa;
            p[0] = (value_type)(sr + ((p[0] * s1a + base_mask) >> base_shift));
            p[1] = (value_type)(sg + ((p[1] * s1a + base_mask) >> base_shift));
            p[2] = (value_type)(sb + ((p[2] * s1a + base_mask) >> base_shift));
            p[3] = (value_type)(sa + p[3] - ((sa * p[3] + base_mask) >> base_shift));
        }
    };

    //======================================================comp_op_ycbcra_dst_over
    template<class ColorT> struct comp_op_ycbcra_dst_over
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Dca + Sca.(1 - Da)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a = base_mask - p[3];
            p[0] = (value_type)(p[0] + ((sr * d1a + base_mask) >> base_shift));
            p[1] = (value_type)(p[1] + ((sg * d1a + base_mask) >> base_shift));
            p[2] = (value_type)(p[2] + ((sb * d1a + base_mask) >> base_shift));
            p[3] = (value_type)(sa + p[3] - ((sa * p[3] + base_mask) >> base_shift));
        }
    };

    //======================================================comp_op_ycbcra_src_in
    template<class ColorT> struct comp_op_ycbcra_src_in
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca.Da
        // Da'  = Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            calc_type da = p[3];
            if(cover < 255)
            {
                unsigned alpha = 255 - cover;
                p[0] = (value_type)(((p[0] * alpha + 255) >> 8) + ((((sr * da + base_mask) >> base_shift) * cover + 255) >> 8));
                p[1] = (value_type)(((p[1] * alpha + 255) >> 8) + ((((sg * da + base_mask) >> base_shift) * cover + 255) >> 8));
                p[2] = (value_type)(((p[2] * alpha + 255) >> 8) + ((((sb * da + base_mask) >> base_shift) * cover + 255) >> 8));
                p[3] = (value_type)(((p[3] * alpha + 255) >> 8) + ((((sa * da + base_mask) >> base_shift) * cover + 255) >> 8));
            }
            else
            {
                p[0] = (value_type)((sr * da + base_mask) >> base_shift);
                p[1] = (value_type)((sg * da + base_mask) >> base_shift);
                p[2] = (value_type)((sb * da + base_mask) >> base_shift);
                p[3] = (value_type)((sa * da + base_mask) >> base_shift);
            }
        }
    };

    //======================================================comp_op_ycbcra_dst_in
    template<class ColorT> struct comp_op_ycbcra_dst_in
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Dca.Sa
        // Da'  = Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned, unsigned, unsigned, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sa = base_mask - ((cover * (base_mask - sa) + 255) >> 8);
            }
            p[0] = (value_type)((p[0] * sa + base_mask) >> base_shift);
            p[1] = (value_type)((p[1] * sa + base_mask) >> base_shift);
            p[2] = (value_type)((p[2] * sa + base_mask) >> base_shift);
            p[3] = (value_type)((p[3] * sa + base_mask) >> base_shift);
        }
    };

    //======================================================comp_op_ycbcra_src_out
    template<class ColorT> struct comp_op_ycbcra_src_out
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca.(1 - Da)
        // Da'  = Sa.(1 - Da) 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            calc_type da = base_mask - p[3];
            if(cover < 255)
            {
                unsigned alpha = 255 - cover;
                p[0] = (value_type)(((p[0] * alpha + 255) >> 8) + ((((sr * da + base_mask) >> base_shift) * cover + 255) >> 8));
                p[1] = (value_type)(((p[1] * alpha + 255) >> 8) + ((((sg * da + base_mask) >> base_shift) * cover + 255) >> 8));
                p[2] = (value_type)(((p[2] * alpha + 255) >> 8) + ((((sb * da + base_mask) >> base_shift) * cover + 255) >> 8));
                p[3] = (value_type)(((p[3] * alpha + 255) >> 8) + ((((sa * da + base_mask) >> base_shift) * cover + 255) >> 8));
            }
            else
            {
                p[0] = (value_type)((sr * da + base_mask) >> base_shift);
                p[1] = (value_type)((sg * da + base_mask) >> base_shift);
                p[2] = (value_type)((sb * da + base_mask) >> base_shift);
                p[3] = (value_type)((sa * da + base_mask) >> base_shift);
            }
        }
    };

    //======================================================comp_op_ycbcra_dst_out
    template<class ColorT> struct comp_op_ycbcra_dst_out
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Dca.(1 - Sa) 
        // Da'  = Da.(1 - Sa) 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned, unsigned, unsigned, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sa = (sa * cover + 255) >> 8;
            }
            sa = base_mask - sa;
            p[0] = (value_type)((p[0] * sa + base_shift) >> base_shift);
            p[1] = (value_type)((p[1] * sa + base_shift) >> base_shift);
            p[2] = (value_type)((p[2] * sa + base_shift) >> base_shift);
            p[3] = (value_type)((p[3] * sa + base_shift) >> base_shift);
        }
    };

    //=====================================================comp_op_ycbcra_src_atop
    template<class ColorT> struct comp_op_ycbcra_src_atop
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca.Da + Dca.(1 - Sa)
        // Da'  = Da
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type da = p[3];
            sa = base_mask - sa;
            p[0] = (value_type)((sr * da + p[0] * sa + base_mask) >> base_shift);
            p[1] = (value_type)((sg * da + p[1] * sa + base_mask) >> base_shift);
            p[2] = (value_type)((sb * da + p[2] * sa + base_mask) >> base_shift);
        }
    };

    //=====================================================comp_op_ycbcra_dst_atop
    template<class ColorT> struct comp_op_ycbcra_dst_atop
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Dca.Sa + Sca.(1 - Da)
        // Da'  = Sa 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            calc_type da = base_mask - p[3];
            if(cover < 255)
            {
                unsigned alpha = 255 - cover;
                sr = (p[0] * sa + sr * da + base_mask) >> base_shift;
                sg = (p[1] * sa + sg * da + base_mask) >> base_shift;
                sb = (p[2] * sa + sb * da + base_mask) >> base_shift;
                p[0] = (value_type)(((p[0] * alpha + 255) >> 8) + ((sr * cover + 255) >> 8));
                p[1] = (value_type)(((p[1] * alpha + 255) >> 8) + ((sg * cover + 255) >> 8));
                p[2] = (value_type)(((p[2] * alpha + 255) >> 8) + ((sb * cover + 255) >> 8));
                p[3] = (value_type)(((p[3] * alpha + 255) >> 8) + ((sa * cover + 255) >> 8));

            }
            else
            {
                p[0] = (value_type)((p[0] * sa + sr * da + base_mask) >> base_shift);
                p[1] = (value_type)((p[1] * sa + sg * da + base_mask) >> base_shift);
                p[2] = (value_type)((p[2] * sa + sb * da + base_mask) >> base_shift);
                p[3] = (value_type)sa;
            }
        }
    };

    //=========================================================comp_op_ycbcra_xor
    template<class ColorT> struct comp_op_ycbcra_xor
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - 2.Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type s1a = base_mask - sa;
            calc_type d1a = base_mask - p[3];
            p[0] = (value_type)((p[0] * s1a + sr * d1a + base_mask) >> base_shift);
            p[1] = (value_type)((p[1] * s1a + sg * d1a + base_mask) >> base_shift);
            p[2] = (value_type)((p[2] * s1a + sb * d1a + base_mask) >> base_shift);
            p[3] = (value_type)(sa + p[3] - ((sa * p[3] + base_mask/2) >> (base_shift - 1)));
        }
    };

    //=========================================================comp_op_ycbcra_plus
    template<class ColorT> struct comp_op_ycbcra_plus
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca + Dca
        // Da'  = Sa + Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type dr = p[0] + sr;
            calc_type dg = p[1] + sg;
            calc_type db = p[2] + sb;
            calc_type da = p[3] + sa;
            p[0] = (dr > base_mask) ? base_mask : dr;
            p[1] = (dg > base_mask) ? base_mask : dg;
            p[2] = (db > base_mask) ? base_mask : db;
            p[3] = (da > base_mask) ? base_mask : da;
        }
    };

    //========================================================comp_op_ycbcra_minus
    template<class ColorT> struct comp_op_ycbcra_minus
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Dca - Sca
        // Da' = 1 - (1 - Sa).(1 - Da)
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type dr = p[0] - sr;
            calc_type dg = p[1] - sg;
            calc_type db = p[2] - sb;
            p[0] = (dr > base_mask) ? 0 : dr;
            p[1] = (dg > base_mask) ? 0 : dg;
            p[2] = (db > base_mask) ? 0 : db;
            p[3] = (value_type)(base_mask - (((base_mask - sa) * (base_mask - p[3]) + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_multiply
    template<class ColorT> struct comp_op_ycbcra_multiply
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type s1a = base_mask - sa;
            calc_type d1a = base_mask - p[3];
            calc_type dr = p[0];
            calc_type dg = p[1];
            calc_type db = p[2];
            p[0] = (value_type)((sr * dr + sr * d1a + dr * s1a + base_mask) >> base_shift);
            p[1] = (value_type)((sg * dg + sg * d1a + dg * s1a + base_mask) >> base_shift);
            p[2] = (value_type)((sb * db + sb * d1a + db * s1a + base_mask) >> base_shift);
            p[3] = (value_type)(sa + p[3] - ((sa * p[3] + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_screen
    template<class ColorT> struct comp_op_ycbcra_screen
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca + Dca - Sca.Dca
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type dr = p[0];
            calc_type dg = p[1];
            calc_type db = p[2];
            calc_type da = p[3];
            p[0] = (value_type)(sr + dr - ((sr * dr + base_mask) >> base_shift));
            p[1] = (value_type)(sg + dg - ((sg * dg + base_mask) >> base_shift));
            p[2] = (value_type)(sb + db - ((sb * db + base_mask) >> base_shift));
            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_overlay
    template<class ColorT> struct comp_op_ycbcra_overlay
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // if 2.Dca < Da
        //   Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //   Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da' = Sa + Da - Sa.Da
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a  = base_mask - p[3];
            calc_type s1a  = base_mask - sa;
            calc_type dr   = p[0];
            calc_type dg   = p[1];
            calc_type db   = p[2];
            calc_type da   = p[3];
            calc_type sada = sa * p[3];

            p[0] = (value_type)(((2*dr < da) ? 
                2*sr*dr + sr*d1a + dr*s1a : 
                sada - 2*(da - dr)*(sa - sr) + sr*d1a + dr*s1a) >> base_shift);

            p[1] = (value_type)(((2*dg < da) ? 
                2*sg*dg + sg*d1a + dg*s1a : 
                sada - 2*(da - dg)*(sa - sg) + sg*d1a + dg*s1a) >> base_shift);

            p[2] = (value_type)(((2*db < da) ? 
                2*sb*db + sb*d1a + db*s1a : 
                sada - 2*(da - db)*(sa - sb) + sb*d1a + db*s1a) >> base_shift);

            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };


    //=====================================================comp_op_ycbcra_darken
    template<class ColorT> struct comp_op_ycbcra_darken
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a = base_mask - p[3];
            calc_type s1a = base_mask - sa;
            calc_type dr  = p[0];
            calc_type dg  = p[1];
            calc_type db  = p[2];
            calc_type da  = p[3];

            p[0] = (value_type)((sd_min(sr * da, dr * sa) + sr * d1a + dr * s1a) >> base_shift);
            p[1] = (value_type)((sd_min(sg * da, dg * sa) + sg * d1a + dg * s1a) >> base_shift);
            p[2] = (value_type)((sd_min(sb * da, db * sa) + sb * d1a + db * s1a) >> base_shift);
            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_lighten
    template<class ColorT> struct comp_op_ycbcra_lighten
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a = base_mask - p[3];
            calc_type s1a = base_mask - sa;
            calc_type dr  = p[0];
            calc_type dg  = p[1];
            calc_type db  = p[2];
            calc_type da  = p[3];

            p[0] = (value_type)((sd_max(sr * da, dr * sa) + sr * d1a + dr * s1a) >> base_shift);
            p[1] = (value_type)((sd_max(sg * da, dg * sa) + sg * d1a + dg * s1a) >> base_shift);
            p[2] = (value_type)((sd_max(sb * da, db * sa) + sb * d1a + db * s1a) >> base_shift);
            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_color_dodge
    template<class ColorT> struct comp_op_ycbcra_color_dodge
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // if Sca.Da + Dca.Sa >= Sa.Da
        //   Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //   Dca' = Dca.Sa/(1-Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        //
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a  = base_mask - p[3];
            calc_type s1a  = base_mask - sa;
            calc_type dr   = p[0];
            calc_type dg   = p[1];
            calc_type db   = p[2];
            calc_type da   = p[3];
            long_type drsa = dr * sa;
            long_type dgsa = dg * sa;
            long_type dbsa = db * sa;
            long_type srda = sr * da;
            long_type sgda = sg * da;
            long_type sbda = sb * da;
            long_type sada = sa * da;

            p[0] = (value_type)((srda + drsa >= sada) ? 
                (sada + sr * d1a + dr * s1a) >> base_shift :
                drsa / (base_mask - (sr << base_shift) / sa) + ((sr * d1a + dr * s1a) >> base_shift));

            p[1] = (value_type)((sgda + dgsa >= sada) ? 
                (sada + sg * d1a + dg * s1a) >> base_shift :
                dgsa / (base_mask - (sg << base_shift) / sa) + ((sg * d1a + dg * s1a) >> base_shift));

            p[2] = (value_type)((sbda + dbsa >= sada) ? 
                (sada + sb * d1a + db * s1a) >> base_shift :
                dbsa / (base_mask - (sb << base_shift) / sa) + ((sb * d1a + db * s1a) >> base_shift));

            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_color_burn
    template<class ColorT> struct comp_op_ycbcra_color_burn
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // if Sca.Da + Dca.Sa <= Sa.Da
        //   Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //   Dca' = Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a  = base_mask - p[3];
            calc_type s1a  = base_mask - sa;
            calc_type dr   = p[0];
            calc_type dg   = p[1];
            calc_type db   = p[2];
            calc_type da   = p[3];
            long_type drsa = dr * sa;
            long_type dgsa = dg * sa;
            long_type dbsa = db * sa;
            long_type srda = sr * da;
            long_type sgda = sg * da;
            long_type sbda = sb * da;
            long_type sada = sa * da;

            p[0] = (value_type)(((srda + drsa <= sada) ? 
                sr * d1a + dr * s1a :
                sa * (srda + drsa - sada) / sr + sr * d1a + dr * s1a) >> base_shift);

            p[1] = (value_type)(((sgda + dgsa <= sada) ? 
                sg * d1a + dg * s1a :
                sa * (sgda + dgsa - sada) / sg + sg * d1a + dg * s1a) >> base_shift);

            p[2] = (value_type)(((sbda + dbsa <= sada) ? 
                sb * d1a + db * s1a :
                sa * (sbda + dbsa - sada) / sb + sb * d1a + db * s1a) >> base_shift);

            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_hard_light
    template<class ColorT> struct comp_op_ycbcra_hard_light
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // if 2.Sca < Sa
        //    Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //    Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da'  = Sa + Da - Sa.Da
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a  = base_mask - p[3];
            calc_type s1a  = base_mask - sa;
            calc_type dr   = p[0];
            calc_type dg   = p[1];
            calc_type db   = p[2];
            calc_type da   = p[3];
            calc_type sada = sa * da;

            p[0] = (value_type)(((2*sr < sa) ? 
                2*sr*dr + sr*d1a + dr*s1a : 
                sada - 2*(da - dr)*(sa - sr) + sr*d1a + dr*s1a) >> base_shift);

            p[1] = (value_type)(((2*sg < sa) ? 
                2*sg*dg + sg*d1a + dg*s1a : 
                sada - 2*(da - dg)*(sa - sg) + sg*d1a + dg*s1a) >> base_shift);

            p[2] = (value_type)(((2*sb < sa) ? 
                2*sb*db + sb*d1a + db*s1a : 
                sada - 2*(da - db)*(sa - sb) + sb*d1a + db*s1a) >> base_shift);

            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_soft_light
    template<class ColorT> struct comp_op_ycbcra_soft_light
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // if 2.Sca < Sa
        //   Dca' = Dca.(Sa + (1 - Dca/Da).(2.Sca - Sa)) + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise if 8.Dca <= Da
        //   Dca' = Dca.(Sa + (1 - Dca/Da).(2.Sca - Sa).(3 - 8.Dca/Da)) + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //   Dca' = (Dca.Sa + ((Dca/Da)^(0.5).Da - Dca).(2.Sca - Sa)) + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da'  = Sa + Da - Sa.Da 

        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned y, unsigned cb, unsigned cr, 
                                         unsigned a, unsigned cover)
        {
            double sr = double(y * cover) / (base_mask * 255);
            double sg = double(cb * cover) / (base_mask * 255);
            double sb = double(cr * cover) / (base_mask * 255);
            double sa = double(a * cover) / (base_mask * 255);
            double dr = double(p[0]) / base_mask;
            double dg = double(p[1]) / base_mask;
            double db = double(p[2]) / base_mask;
            double da = double(p[3] ? p[3] : 1) / base_mask;
            if(cover < 255)
            {
                a = (a * cover + 255) >> 8;
            }

            if(2*sr < sa)       dr = dr*(sa + (1 - dr/da)*(2*sr - sa)) + sr*(1 - da) + dr*(1 - sa);
            else if(8*dr <= da) dr = dr*(sa + (1 - dr/da)*(2*sr - sa)*(3 - 8*dr/da)) + sr*(1 - da) + dr*(1 - sa);
            else                dr = (dr*sa + (sqrt(dr/da)*da - dr)*(2*sr - sa)) + sr*(1 - da) + dr*(1 - sa);

            if(2*sg < sa)       dg = dg*(sa + (1 - dg/da)*(2*sg - sa)) + sg*(1 - da) + dg*(1 - sa);
            else if(8*dg <= da) dg = dg*(sa + (1 - dg/da)*(2*sg - sa)*(3 - 8*dg/da)) + sg*(1 - da) + dg*(1 - sa);
            else                dg = (dg*sa + (sqrt(dg/da)*da - dg)*(2*sg - sa)) + sg*(1 - da) + dg*(1 - sa);

            if(2*sb < sa)       db = db*(sa + (1 - db/da)*(2*sb - sa)) + sb*(1 - da) + db*(1 - sa);
            else if(8*db <= da) db = db*(sa + (1 - db/da)*(2*sb - sa)*(3 - 8*db/da)) + sb*(1 - da) + db*(1 - sa);
            else                db = (db*sa + (sqrt(db/da)*da - db)*(2*sb - sa)) + sb*(1 - da) + db*(1 - sa);

            p[0] = (value_type)uround(dr * base_mask);
            p[1] = (value_type)uround(dg * base_mask);
            p[2] = (value_type)uround(db * base_mask);
            p[3] = (value_type)(a + p[3] - ((a * p[3] + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_difference
    template<class ColorT> struct comp_op_ycbcra_difference
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type dr = p[0];
            calc_type dg = p[1];
            calc_type db = p[2];
            calc_type da = p[3];
            p[0] = (value_type)(sr + dr - ((2 * sd_min(sr*da, dr*sa)) >> base_shift));
            p[1] = (value_type)(sg + dg - ((2 * sd_min(sg*da, dg*sa)) >> base_shift));
            p[2] = (value_type)(sb + db - ((2 * sd_min(sb*da, db*sa)) >> base_shift));
            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_exclusion
    template<class ColorT> struct comp_op_ycbcra_exclusion
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            calc_type d1a = base_mask - p[3];
            calc_type s1a = base_mask - sa;
            calc_type dr = p[0];
            calc_type dg = p[1];
            calc_type db = p[2];
            calc_type da = p[3];
            p[0] = (value_type)((sr*da + dr*sa - 2*sr*dr + sr*d1a + dr*s1a) >> base_shift);
            p[1] = (value_type)((sg*da + dg*sa - 2*sg*dg + sg*d1a + dg*s1a) >> base_shift);
            p[2] = (value_type)((sb*da + db*sa - 2*sb*db + sb*d1a + db*s1a) >> base_shift);
            p[3] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
        }
    };

    //=====================================================comp_op_ycbcra_contrast
    template<class ColorT> struct comp_op_ycbcra_contrast
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };


        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if(cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            long_type dr = p[0];
            long_type dg = p[1];
            long_type db = p[2];
            int       da = p[3];
            long_type d2a = da >> 1;
            unsigned s2a = sa >> 1;

            int y = (int)((((dr - d2a) * int((sr - s2a)*2 + base_mask)) >> base_shift) + d2a); 
            int cb = (int)((((dg - d2a) * int((sg - s2a)*2 + base_mask)) >> base_shift) + d2a); 
            int cr = (int)((((db - d2a) * int((sb - s2a)*2 + base_mask)) >> base_shift) + d2a); 

            y = (y < 0) ? 0 : y;
            cb = (cb < 0) ? 0 : cb;
            cr = (cr < 0) ? 0 : cr;

            p[0] = (value_type)((y > da) ? da : y);
            p[1] = (value_type)((cb > da) ? da : cb);
            p[2] = (value_type)((cr > da) ? da : cr);
        }
    };











    //======================================================comp_op_table_ycbcra
    template<class ColorT> struct comp_op_table_ycbcra
    {
        typedef typename ColorT::value_type value_type;
        typedef void (*comp_op_func_type)(value_type* p, 
                                          unsigned cy, 
                                          unsigned ccb, 
                                          unsigned ccr,
                                          unsigned ca,
                                          unsigned cover);
        static comp_op_func_type g_comp_op_func[];
    };

    //==========================================================g_comp_op_func
    template<class ColorT> 
    typename comp_op_table_ycbcra<ColorT>::comp_op_func_type
    comp_op_table_ycbcra<ColorT>::g_comp_op_func[] = 
    {
        comp_op_ycbcra_clear      <ColorT>::blend_pix,
        comp_op_ycbcra_src        <ColorT>::blend_pix,
        comp_op_ycbcra_dst        <ColorT>::blend_pix,
        comp_op_ycbcra_src_over   <ColorT>::blend_pix,
        comp_op_ycbcra_dst_over   <ColorT>::blend_pix,
        comp_op_ycbcra_src_in     <ColorT>::blend_pix,
        comp_op_ycbcra_dst_in     <ColorT>::blend_pix,
        comp_op_ycbcra_src_out    <ColorT>::blend_pix,
        comp_op_ycbcra_dst_out    <ColorT>::blend_pix,
        comp_op_ycbcra_src_atop   <ColorT>::blend_pix,
        comp_op_ycbcra_dst_atop   <ColorT>::blend_pix,
        comp_op_ycbcra_xor        <ColorT>::blend_pix,
        comp_op_ycbcra_plus       <ColorT>::blend_pix,
        comp_op_ycbcra_minus      <ColorT>::blend_pix,
        comp_op_ycbcra_multiply   <ColorT>::blend_pix,
        comp_op_ycbcra_screen     <ColorT>::blend_pix,
        comp_op_ycbcra_overlay    <ColorT>::blend_pix,
        comp_op_ycbcra_darken     <ColorT>::blend_pix,
        comp_op_ycbcra_lighten    <ColorT>::blend_pix,
        comp_op_ycbcra_color_dodge<ColorT>::blend_pix,
        comp_op_ycbcra_color_burn <ColorT>::blend_pix,
        comp_op_ycbcra_hard_light <ColorT>::blend_pix,
        comp_op_ycbcra_soft_light <ColorT>::blend_pix,
        comp_op_ycbcra_difference <ColorT>::blend_pix,
        comp_op_ycbcra_exclusion  <ColorT>::blend_pix,
        comp_op_ycbcra_contrast   <ColorT>::blend_pix,
        0
    };


    //====================================================comp_op_adaptor_ycbcra
    template<class ColorT> struct comp_op_adaptor_ycbcra
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            comp_op_table_ycbcra<ColorT>::g_comp_op_func[op]
                (p, (cy * ca + base_mask) >> base_shift, 
                    (ccb * ca + base_mask) >> base_shift,
                    (ccr * ca + base_mask) >> base_shift,
                     ca, cover);
        }
    };

    //=========================================comp_op_adaptor_clip_to_dst_ycbcra
    template<class ColorT> struct comp_op_adaptor_clip_to_dst_ycbcra
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            cy = (cy * ca + base_mask) >> base_shift;
            ccb = (ccb * ca + base_mask) >> base_shift;
            ccr = (ccr * ca + base_mask) >> base_shift;
            unsigned da = p[3];
            comp_op_table_ycbcra<ColorT>::g_comp_op_func[op]
                (p, (cy * da + base_mask) >> base_shift, 
                    (ccb * da + base_mask) >> base_shift, 
                    (ccr * da + base_mask) >> base_shift, 
                    (ca * da + base_mask) >> base_shift, 
                    cover);
        }
    };

    //================================================comp_op_adaptor_ycbcra_pre
    template<class ColorT> struct comp_op_adaptor_ycbcra_pre
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            comp_op_table_ycbcra<ColorT>::g_comp_op_func[op](p, cy, ccb, ccr, ca, cover);
        }
    };

    //=====================================comp_op_adaptor_clip_to_dst_ycbcra_pre
    template<class ColorT> struct comp_op_adaptor_clip_to_dst_ycbcra_pre
    {
        typedef ColorT color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            unsigned da = p[3];
            comp_op_table_ycbcra<ColorT>::g_comp_op_func[op]
                (p, (cy * da + base_mask) >> base_shift, 
                    (ccb * da + base_mask) >> base_shift, 
                    (ccr * da + base_mask) >> base_shift, 
                    (ca * da + base_mask) >> base_shift, 
                    cover);
        }
    };

    //=======================================================comp_adaptor_ycbcra
    template<class BlenderPre> struct comp_adaptor_ycbcra
    {
        typedef typename BlenderPre::color_type color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            BlenderPre::blend_pix(p, 
                                  (cy * ca + base_mask) >> base_shift, 
                                  (ccb * ca + base_mask) >> base_shift,
                                  (ccr * ca + base_mask) >> base_shift,
                                  ca, cover);
        }
    };

    //==========================================comp_adaptor_clip_to_dst_ycbcra
    template<class BlenderPre> struct comp_adaptor_clip_to_dst_ycbcra
    {
        typedef typename BlenderPre::color_type color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            cy = (cy * ca + base_mask) >> base_shift;
            ccb = (ccb * ca + base_mask) >> base_shift;
            ccr = (ccr * ca + base_mask) >> base_shift;
            unsigned da = p[3];
            BlenderPre::blend_pix(p, 
                                  (cy * da + base_mask) >> base_shift, 
                                  (ccb * da + base_mask) >> base_shift, 
                                  (ccr * da + base_mask) >> base_shift, 
                                  (ca * da + base_mask) >> base_shift, 
                                  cover);
        }
    };

    //======================================comp_adaptor_clip_to_dst_ycbcra_pre
    template<class BlenderPre> struct comp_adaptor_clip_to_dst_ycbcra_pre
    {
        typedef typename BlenderPre::color_type color_type;
        typedef typename color_type::value_type value_type;
        enum base_scale_e
        {  
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask 
        };

        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
                                         unsigned cy, unsigned ccb, unsigned ccr,
                                         unsigned ca,
                                         unsigned cover)
        {
            unsigned da = p[3];
            BlenderPre::blend_pix(p, 
                                  (cy * da + base_mask) >> base_shift, 
                                  (ccb * da + base_mask) >> base_shift, 
                                  (ccr * da + base_mask) >> base_shift, 
                                  (ca * da + base_mask) >> base_shift, 
                                  cover);
        }
    };






    //===============================================copy_or_blend_ycbcra_wrapper
    template<class Blender> struct copy_or_blend_ycbcra_wrapper
    {
        typedef typename Blender::color_type color_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        {
            base_shift = color_type::base_shift,
            base_scale = color_type::base_scale,
            base_mask  = color_type::base_mask
        };

        //--------------------------------------------------------------------
        static AGG_INLINE void copy_or_blend_pix(value_type* p, 
                                                 unsigned cy, unsigned ccb, unsigned ccr,
                                                 unsigned alpha)
        {
            if(alpha)
            {
                if(alpha == base_mask)
                {
                    p[0] = cy;
                    p[1] = ccb;
                    p[2] = ccr;
                    p[3] = base_mask;
                }
                else
                {
                    Blender::blend_pix(p, cy, ccb, ccr, alpha);
                }
            }
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void copy_or_blend_pix(value_type* p, 
                                                 unsigned cy, unsigned ccb, unsigned ccr,
                                                 unsigned alpha,
                                                 unsigned cover)
        {
            if(cover == 255)
            {
                copy_or_blend_pix(p, cy, ccb, ccr, alpha);
            }
            else
            {
                if(alpha)
                {
                    alpha = (alpha * (cover + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = cy;
                        p[1] = ccb;
                        p[2] = ccr;
                        p[3] = base_mask;
                    }
                    else
                    {
                        Blender::blend_pix(p, cy, ccb, ccr, alpha, cover);
                    }
                }
            }
        }
    };





    
    //=================================================pixfmt_alpha_blend_ycbcra
    template<class Blender, class RenBuf, class PixelT = int32u> 
    class pixfmt_alpha_blend_ycbcra
    {
    public:
        typedef RenBuf   rbuf_type;
        typedef typename rbuf_type::row_data row_data;
        typedef PixelT   pixel_type;
        typedef Blender  blender_type;
        typedef typename blender_type::color_type color_type;
        typedef int                               order_type; // A fake one
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef copy_or_blend_ycbcra_wrapper<blender_type> cob_type;
        enum base_scale_e
        {
            base_shift = color_type::base_shift,
            base_scale = color_type::base_scale,
            base_mask  = color_type::base_mask,
            pix_width  = sizeof(pixel_type)
        };

        //--------------------------------------------------------------------
        pixfmt_alpha_blend_ycbcra() : m_rbuf(0) {}
        pixfmt_alpha_blend_ycbcra(rbuf_type& rb) : m_rbuf(&rb) {}
        void attach(rbuf_type& rb) { m_rbuf = &rb; }

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
        row_data row(int y) const
        {
            return m_rbuf->row(y);
        }

        //--------------------------------------------------------------------
        AGG_INLINE static void make_pix(int8u* p, const color_type& c)
        {
            ((value_type*)p)[0] = c.y;
            ((value_type*)p)[1] = c.cb;
            ((value_type*)p)[2] = c.cr;
            ((value_type*)p)[3] = c.a;
        }

        //--------------------------------------------------------------------
        AGG_INLINE color_type pixel(int x, int y) const
        {
            const value_type* p = (const value_type*)m_rbuf->row_ptr(y);
            if(p)
            {
                p += x << 2;
                return color_type(p[0], 
                                  p[1], 
                                  p[2], 
                                  p[3]);
            }
            return color_type::no_color();
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_pixel(int x, int y, const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, 1) + (x << 2);
            p[0] = c.y;
            p[1] = c.cb;
            p[2] = c.cr;
            p[3] = c.a;
        }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            cob_type::copy_or_blend_pix(
                (value_type*)m_rbuf->row_ptr(x, y, 1) + (x << 2), 
                c.y, c.cb, c.cr, c.a, 
                cover);
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_hline(int x, int y, 
                                   unsigned len, 
                                   const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            pixel_type v;
            ((value_type*)&v)[0] = c.y;
            ((value_type*)&v)[1] = c.cb;
            ((value_type*)&v)[2] = c.cr;
            ((value_type*)&v)[3] = c.a;
            do
            {
                *(pixel_type*)p = v;
                p += 4;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_vline(int x, int y,
                                   unsigned len, 
                                   const color_type& c)
        {
            pixel_type v;
            ((value_type*)&v)[0] = c.y;
            ((value_type*)&v)[1] = c.cb;
            ((value_type*)&v)[2] = c.cr;
            ((value_type*)&v)[3] = c.a;
            do
            {
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                *(pixel_type*)p = v;
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
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    pixel_type v;
                    ((value_type*)&v)[0] = c.y;
                    ((value_type*)&v)[1] = c.cb;
                    ((value_type*)&v)[2] = c.cr;
                    ((value_type*)&v)[3] = c.a;
                    do
                    {
                        *(pixel_type*)p = v;
                        p += 4;
                    }
                    while(--len);
                }
                else
                {
                    if(cover == 255)
                    {
                        do
                        {
                            blender_type::blend_pix(p, c.y, c.cb, c.cr, alpha);
                            p += 4;
                        }
                        while(--len);
                    }
                    else
                    {
                        do
                        {
                            blender_type::blend_pix(p, c.y, c.cb, c.cr, alpha, cover);
                            p += 4;
                        }
                        while(--len);
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
                value_type* p;
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    pixel_type v;
                    ((value_type*)&v)[0] = c.y;
                    ((value_type*)&v)[1] = c.cb;
                    ((value_type*)&v)[2] = c.cr;
                    ((value_type*)&v)[3] = c.a;
                    do
                    {
                        *(pixel_type*)p = v;
                        p = (value_type*)m_rbuf->next_row(p);
                    }
                    while(--len);
                }
                else
                {
                    if(cover == 255)
                    {
                        do
                        {
                            p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                            blender_type::blend_pix(p, c.y, c.cb, c.cr, alpha);
                        }
                        while(--len);
                    }
                    else
                    {
                        do
                        {
                            p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                            blender_type::blend_pix(p, c.y, c.cb, c.cr, alpha, cover);
                        }
                        while(--len);
                    }
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
                value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
                do 
                {
                    calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cb;
                        p[2] = c.cr;
                        p[3] = base_mask;
                    }
                    else
                    {
                        blender_type::blend_pix(p, c.y, c.cb, c.cr, alpha, *covers);
                    }
                    p += 4;
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
                    value_type* p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                    calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                    if(alpha == base_mask)
                    {
                        p[0] = c.y;
                        p[1] = c.cb;
                        p[2] = c.cr;
                        p[3] = base_mask;
                    }
                    else
                    {
                        blender_type::blend_pix(p, c.y, c.cb, c.cr, alpha, *covers);
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
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            do 
            {
                p[0] = colors->y;
                p[1] = colors->cb;
                p[2] = colors->cr;
                p[3] = colors->a;
                ++colors;
                p += 4;
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
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            if(covers)
            {
                do 
                {
                    cob_type::copy_or_blend_pix(p, 
                                                colors->y, 
                                                colors->cb, 
                                                colors->cr, 
                                                colors->a, 
                                                *covers++);
                    p += 4;
                    ++colors;
                }
                while(--len);
            }
            else
            {
                if(cover == 255)
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->y, 
                                                    colors->cb, 
                                                    colors->cr, 
                                                    colors->a);
                        p += 4;
                        ++colors;
                    }
                    while(--len);
                }
                else
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->y, 
                                                    colors->cb, 
                                                    colors->cr, 
                                                    colors->a, 
                                                    cover);
                        p += 4;
                        ++colors;
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
                    p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                    cob_type::copy_or_blend_pix(p, 
                                                colors->y, 
                                                colors->cb, 
                                                colors->cr, 
                                                colors->a,
                                                *covers++);
                    ++colors;
                }
                while(--len);
            }
            else
            {
                if(cover == 255)
                {
                    do 
                    {
                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->y, 
                                                    colors->cb, 
                                                    colors->cr, 
                                                    colors->a);
                        ++colors;
                    }
                    while(--len);
                }
                else
                {
                    do 
                    {
                        p = (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2);
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->y, 
                                                    colors->cb, 
                                                    colors->cr, 
                                                    colors->a, 
                                                    cover);
                        ++colors;
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
                row_data y = m_rbuf->row(y);
                if(y.ptr)
                {
                    unsigned len = y.x2 - y.x1 + 1;
                    value_type* p = 
                        (value_type*)m_rbuf->row_ptr(y.x1, y, len) + (y.x1 << 2);
                    do
                    {
                        f(p);
                        p += 4;
                    }
                    while(--len);
                }
            }
        }

        //--------------------------------------------------------------------
        void premultiply()
        {
            for_each_pixel(multiplier_ycbcra<color_type>::premultiply);
        }

        //--------------------------------------------------------------------
        void demultiply()
        {
            for_each_pixel(multiplier_ycbcra<color_type>::demultiply);
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_dir(const GammaLut& cb)
        {
            for_each_pixel(apply_gamma_dir_ycbcr<color_type, GammaLut>(cb));
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_inv(const GammaLut& cb)
        {
            for_each_pixel(apply_gamma_inv_ycbcr<color_type, GammaLut>(cb));
        }

        //--------------------------------------------------------------------
        template<class RenBuf2> void copy_from(const RenBuf2& from, 
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
            const value_type* psrc = (value_type*)from.row_ptr(ysrc);
            if(psrc)
            {
                psrc += xsrc << 2;
                value_type* pdst = 
                    (value_type*)m_rbuf->row_ptr(xdst, ydst, len) + (xdst << 2);
                int incp = 4;
                if(xdst > xsrc)
                {
                    psrc += (len-1) << 2;
                    pdst += (len-1) << 2;
                    incp = -4;
                }

                if(cover == 255)
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(pdst, 
                                                    psrc[0],
                                                    psrc[1],
                                                    psrc[2],
                                                    psrc[3]);
                        psrc += incp;
                        pdst += incp;
                    }
                    while(--len);
                }
                else
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(pdst, 
                                                    psrc[0],
                                                    psrc[1],
                                                    psrc[2],
                                                    psrc[3],
                                                    cover);
                        psrc += incp;
                        pdst += incp;
                    }
                    while(--len);
                }
            }
        }

    private:
        rbuf_type* m_rbuf;
    };




    //================================================pixfmt_custom_blend_ycbcra
    template<class Blender, class RenBuf> class pixfmt_custom_blend_ycbcra
    {
    public:
        typedef RenBuf   rbuf_type;
        typedef typename rbuf_type::row_data row_data;
        typedef Blender  blender_type;
        typedef typename blender_type::color_type color_type;
        typedef int                               order_type; // A fake one
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum base_scale_e
        {
            base_shift = color_type::base_shift,
            base_scale = color_type::base_scale,
            base_mask  = color_type::base_mask,
            pix_width  = sizeof(value_type) * 4 
        };


        //--------------------------------------------------------------------
        pixfmt_custom_blend_ycbcra() : m_rbuf(0), m_comp_op(3) {}
        pixfmt_custom_blend_ycbcra(rbuf_type& rb, unsigned comp_op=3) : 
            m_rbuf(&rb),
            m_comp_op(comp_op)
        {}
        void attach(rbuf_type& rb) { m_rbuf = &rb; }

        //--------------------------------------------------------------------
        unsigned width()  const { return m_rbuf->width();  }
        unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        void comp_op(unsigned op) { m_comp_op = op; }
        unsigned comp_op() const  { return m_comp_op; }

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
            ((value_type*)p)[0] = c.y;
            ((value_type*)p)[1] = c.cb;
            ((value_type*)p)[2] = c.cr;
            ((value_type*)p)[3] = c.a;
        }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y) const
        {
            const value_type* p = (value_type*)m_rbuf->row_ptr(y) + (x << 2);
            return color_type(p[0], 
                              p[1], 
                              p[2], 
                              p[3]);
        }

        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            blender_type::blend_pix(
                m_comp_op, 
                (value_type*)m_rbuf->row_ptr(x, y, 1) + (x << 2), 
                c.y, c.cb, c.cr, c.a, 255);
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            blender_type::blend_pix(
                m_comp_op, 
                (value_type*)m_rbuf->row_ptr(x, y, 1) + (x << 2),
                c.y, c.cb, c.cr, c.a, 
                cover);
        }

        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);;
            do
            {
                blender_type::blend_pix(m_comp_op, p, c.y, c.cb, c.cr, c.a, 255);
                p += 4;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void copy_vline(int x, int y, unsigned len, const color_type& c)
        {
            do
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2),
                    c.y, c.cb, c.cr, c.a, 255);
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {

            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            do
            {
                blender_type::blend_pix(m_comp_op, p, c.y, c.cb, c.cr, c.a, cover);
                p += 4;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {

            do
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2), 
                    c.y, c.cb, c.cr, c.a, 
                    cover);
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            do 
            {
                blender_type::blend_pix(m_comp_op, 
                                        p, c.y, c.cb, c.cr, c.a, 
                                        *covers++);
                p += 4;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            do 
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2), 
                    c.y, c.cb, c.cr, c.a, 
                    *covers++);
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void copy_color_hspan(int x, int y,
                              unsigned len, 
                              const color_type* colors)
        {

            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            do 
            {
                p[0] = colors->y;
                p[1] = colors->cb;
                p[2] = colors->cr;
                p[3] = colors->a;
                ++colors;
                p += 4;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p = (value_type*)m_rbuf->row_ptr(x, y, len) + (x << 2);
            do 
            {
                blender_type::blend_pix(m_comp_op, 
                                        p, 
                                        colors->y, 
                                        colors->cb, 
                                        colors->cr, 
                                        colors->a, 
                                        covers ? *covers++ : cover);
                p += 4;
                ++colors;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            do 
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    (value_type*)m_rbuf->row_ptr(x, y++, 1) + (x << 2), 
                    colors->y,
                    colors->cb,
                    colors->cr,
                    colors->a,
                    covers ? *covers++ : cover);
                ++colors;
            }
            while(--len);

        }

        //--------------------------------------------------------------------
        template<class Function> void for_each_pixel(Function f)
        {
            unsigned y;
            for(y = 0; y < height(); ++y)
            {
                row_data y = m_rbuf->row(y);
                if(y.ptr)
                {
                    unsigned len = y.x2 - y.x1 + 1;
                    value_type* p = 
                        (value_type*)m_rbuf->row_ptr(y.x1, y, len) + (y.x1 << 2);
                    do
                    {
                        f(p);
                        p += 4;
                    }
                    while(--len);
                }
            }
        }

        //--------------------------------------------------------------------
        void premultiply()
        {
            for_each_pixel(multiplier_ycbcra<color_type>::premultiply);
        }

        //--------------------------------------------------------------------
        void demultiply()
        {
            for_each_pixel(multiplier_ycbcra<color_type>::demultiply);
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_dir(const GammaLut& cb)
        {
            for_each_pixel(apply_gamma_dir_ycbcr<color_type, GammaLut>(cb));
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_inv(const GammaLut& cb)
        {
            for_each_pixel(apply_gamma_inv_ycbcr<color_type, GammaLut>(cb));
        }

        //--------------------------------------------------------------------
        template<class RenBuf2> void copy_from(const RenBuf2& from, 
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
                psrc += xsrc << 2;
                value_type* pdst = 
                    (value_type*)m_rbuf->row_ptr(xdst, ydst, len) + (xdst << 2);

                int incp = 4;
                if(xdst > xsrc)
                {
                    psrc += (len-1) << 2;
                    pdst += (len-1) << 2;
                    incp = -4;
                }

                do 
                {
                    blender_type::blend_pix(m_comp_op, 
                                            pdst, 
                                            psrc[0],
                                            psrc[1],
                                            psrc[2],
                                            psrc[3],
                                            cover);
                    psrc += incp;
                    pdst += incp;
                }
                while(--len);
            }
        }

    private:
        rbuf_type* m_rbuf;
        unsigned m_comp_op;
    };



    typedef pixfmt_alpha_blend_ycbcra<blender_ycbcra<ycbcra8>, rendering_buffer> pixfmt_ycbcra;    //----pixfmt_ycbcra


    //-----------------------------------------------------------------------
//    typedef blender_ycbcra<ycbcra8, order_ycbcra> blender_ycbcra32; //----blender_ycbcra32
//    typedef blender_ycbcra<ycbcra8, order_argb> blender_argb32; //----blender_argb32
//    typedef blender_ycbcra<ycbcra8, order_abgr> blender_abgr32; //----blender_abgr32
//    typedef blender_ycbcra<ycbcra8, order_bgra> blender_bgra32; //----blender_bgra32
//
//    typedef blender_ycbcra_pre<ycbcra8, order_ycbcra> blender_ycbcra32_pre; //----blender_ycbcra32_pre
//    typedef blender_ycbcra_pre<ycbcra8, order_argb> blender_argb32_pre; //----blender_argb32_pre
//    typedef blender_ycbcra_pre<ycbcra8, order_abgr> blender_abgr32_pre; //----blender_abgr32_pre
//    typedef blender_ycbcra_pre<ycbcra8, order_bgra> blender_bgra32_pre; //----blender_bgra32_pre
//
//    typedef blender_ycbcra_plain<ycbcra8, order_ycbcra> blender_ycbcra32_plain; //----blender_ycbcra32_plain
//    typedef blender_ycbcra_plain<ycbcra8, order_argb> blender_argb32_plain; //----blender_argb32_plain
//    typedef blender_ycbcra_plain<ycbcra8, order_abgr> blender_abgr32_plain; //----blender_abgr32_plain
//    typedef blender_ycbcra_plain<ycbcra8, order_bgra> blender_bgra32_plain; //----blender_bgra32_plain
//
//    typedef blender_ycbcra<ycbcra16, order_ycbcra> blender_ycbcra64; //----blender_ycbcra64
//    typedef blender_ycbcra<ycbcra16, order_argb> blender_argb64; //----blender_argb64
//    typedef blender_ycbcra<ycbcra16, order_abgr> blender_abgr64; //----blender_abgr64
//    typedef blender_ycbcra<ycbcra16, order_bgra> blender_bgra64; //----blender_bgra64
//
//    typedef blender_ycbcra_pre<ycbcra16, order_ycbcra> blender_ycbcra64_pre; //----blender_ycbcra64_pre
//    typedef blender_ycbcra_pre<ycbcra16, order_argb> blender_argb64_pre; //----blender_argb64_pre
//    typedef blender_ycbcra_pre<ycbcra16, order_abgr> blender_abgr64_pre; //----blender_abgr64_pre
//    typedef blender_ycbcra_pre<ycbcra16, order_bgra> blender_bgra64_pre; //----blender_bgra64_pre
//
//
//    //-----------------------------------------------------------------------
//    typedef int32u pixel32_type;
//    typedef pixfmt_alpha_blend_ycbcra<blender_ycbcra32, rendering_buffer, pixel32_type> pixfmt_ycbcra32; //----pixfmt_ycbcra32
//    typedef pixfmt_alpha_blend_ycbcra<blender_argb32, rendering_buffer, pixel32_type> pixfmt_argb32; //----pixfmt_argb32
//    typedef pixfmt_alpha_blend_ycbcra<blender_abgr32, rendering_buffer, pixel32_type> pixfmt_abgr32; //----pixfmt_abgr32
//    typedef pixfmt_alpha_blend_ycbcra<blender_bgra32, rendering_buffer, pixel32_type> pixfmt_bgra32; //----pixfmt_bgra32
//
//    typedef pixfmt_alpha_blend_ycbcra<blender_ycbcra32_pre, rendering_buffer, pixel32_type> pixfmt_ycbcra32_pre; //----pixfmt_ycbcra32_pre
//    typedef pixfmt_alpha_blend_ycbcra<blender_argb32_pre, rendering_buffer, pixel32_type> pixfmt_argb32_pre; //----pixfmt_argb32_pre
//    typedef pixfmt_alpha_blend_ycbcra<blender_abgr32_pre, rendering_buffer, pixel32_type> pixfmt_abgr32_pre; //----pixfmt_abgr32_pre
//    typedef pixfmt_alpha_blend_ycbcra<blender_bgra32_pre, rendering_buffer, pixel32_type> pixfmt_bgra32_pre; //----pixfmt_bgra32_pre
//
//    typedef pixfmt_alpha_blend_ycbcra<blender_ycbcra32_plain, rendering_buffer, pixel32_type> pixfmt_ycbcra32_plain; //----pixfmt_ycbcra32_plain
//    typedef pixfmt_alpha_blend_ycbcra<blender_argb32_plain, rendering_buffer, pixel32_type> pixfmt_argb32_plain; //----pixfmt_argb32_plain
//    typedef pixfmt_alpha_blend_ycbcra<blender_abgr32_plain, rendering_buffer, pixel32_type> pixfmt_abgr32_plain; //----pixfmt_abgr32_plain
//    typedef pixfmt_alpha_blend_ycbcra<blender_bgra32_plain, rendering_buffer, pixel32_type> pixfmt_bgra32_plain; //----pixfmt_bgra32_plain
//
//    struct  pixel64_type { int16u c[4]; };
//    typedef pixfmt_alpha_blend_ycbcra<blender_ycbcra64, rendering_buffer, pixel64_type> pixfmt_ycbcra64; //----pixfmt_ycbcra64
//    typedef pixfmt_alpha_blend_ycbcra<blender_argb64, rendering_buffer, pixel64_type> pixfmt_argb64; //----pixfmt_argb64
//    typedef pixfmt_alpha_blend_ycbcra<blender_abgr64, rendering_buffer, pixel64_type> pixfmt_abgr64; //----pixfmt_abgr64
//    typedef pixfmt_alpha_blend_ycbcra<blender_bgra64, rendering_buffer, pixel64_type> pixfmt_bgra64; //----pixfmt_bgra64
//
//    typedef pixfmt_alpha_blend_ycbcra<blender_ycbcra64_pre, rendering_buffer, pixel64_type> pixfmt_ycbcra64_pre; //----pixfmt_ycbcra64_pre
//    typedef pixfmt_alpha_blend_ycbcra<blender_argb64_pre, rendering_buffer, pixel64_type> pixfmt_argb64_pre; //----pixfmt_argb64_pre
//    typedef pixfmt_alpha_blend_ycbcra<blender_abgr64_pre, rendering_buffer, pixel64_type> pixfmt_abgr64_pre; //----pixfmt_abgr64_pre
//    typedef pixfmt_alpha_blend_ycbcra<blender_bgra64_pre, rendering_buffer, pixel64_type> pixfmt_bgra64_pre; //----pixfmt_bgra64_pre
}

#endif

