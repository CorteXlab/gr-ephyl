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

#ifndef INCLUDED_EPHYL_TURBOFSK_TX_IMPL_H
#define INCLUDED_EPHYL_TURBOFSK_TX_IMPL_H

#include <ephyl/turbofsk_tx.h>
#include <libTurboFSK_v3.h>


namespace gr {
  namespace ephyl {

    class turbofsk_tx_impl : public turbofsk_tx
    {
     private:

      mxArray *my_in; /* Define input parameters */
      mxArray *outTx = NULL;    /* and output parameters to be passed to the library functions */
      int NbBits,Signal_len,pow2_buffer;
      double *a,*b;
      size_t a_size,b_size;
    
     public:
      turbofsk_tx_impl();
      ~turbofsk_tx_impl();


      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace ephyl
} // namespace gr

#endif /* INCLUDED_EPHYL_TURBOFSK_TX_IMPL_H */

