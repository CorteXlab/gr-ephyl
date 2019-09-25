/* -*- c++ -*- */
/* 
 * Copyright 2019 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_EPHYL_TURBOFSK_RX_IMPL_H
#define INCLUDED_EPHYL_TURBOFSK_RX_IMPL_H

#include <ephyl/turbofsk_rx.h>
#include <libTurboFSK_v2.h>


namespace gr {
  namespace ephyl {

    class turbofsk_rx_impl : public turbofsk_rx
    {
     private:

      mxArray *rx_in;  /* Define input parameters  */
      mxArray *mxNbBits;
      mxArray *outRxBits = NULL;
      mxArray *outcrcCheck = NULL;
      int NbErr,Signal_len,NbBits,pow2_buffer,cnt;
      double *d;
      size_t c_size,d_size;
    
     public:
      turbofsk_rx_impl();
      ~turbofsk_rx_impl();


      // Where all the action really happens
      // void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace ephyl
} // namespace gr

#endif /* INCLUDED_EPHYL_TURBOFSK_RX_IMPL_H */
