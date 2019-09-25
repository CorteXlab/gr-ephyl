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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "turbofsk_rx_impl.h"
#include <algorithm>
#include "init_turbofsk.h"

// using namespace std;
// extern bool libturbofsk_init = true;

// int lib_turbofsk_init_rx = 0;
// std::mutex init_mutex_rx;

namespace gr {
  namespace ephyl {
    
    turbofsk_rx::sptr
    turbofsk_rx::make()
    {
      return gnuradio::get_initial_sptr
        (new turbofsk_rx_impl());
    }

    /*
     * The private constructor
     */
    turbofsk_rx_impl::turbofsk_rx_impl()
      : gr::block("TurboFSK RX",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(unsigned char)))
    {
      get_turbofsk();

      // in EPHYL framework, the packet size is:
      // 14 payload chars + tab + slot_n char = 16 chars = 128 bits 
      NbBits = 128; 
      /*  Signal_len = (64*32)+(1+(NbBits+16)/8)*4*137+(1+int((1+(NbBits+16)/8)*4/5))*137 */
      Signal_len = 14652;      
      cnt = 0;

      /* Create the input data */
      rx_in = mxCreateDoubleMatrix(1,Signal_len*2,mxREAL);    // Take input twice, to 
      d = mxGetPr(rx_in);
      d_size = mxGetN(rx_in);

      mxNbBits = mxCreateDoubleMatrix(1,1,mxREAL);
      double *bits = mxGetPr(mxNbBits);
      *bits = NbBits ;


      set_min_output_buffer(0,NbBits);
    }

    /*
     * Our virtual destructor.
     */
    turbofsk_rx_impl::~turbofsk_rx_impl()
    {
      mxDestroyArray(rx_in);
      mxDestroyArray(outRxBits);
      mxDestroyArray(outcrcCheck);
      mxDestroyArray(mxNbBits);
      release_turbofsk();
    }

    // void
    // turbofsk_rx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    // {
    //   ninput_items_required[0] = Signal_len;
    // }

    int
    turbofsk_rx_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {

      const float *in = (const float *) input_items[0];
      unsigned char *out = (unsigned char *) output_items[0];

      // printf("\nNINPUT: %d\n",ninput_items[0]);
      printf("\navant fill buffer cnt: %d\n",cnt);

      int index_input;
      for (index_input=0;
           index_input < ninput_items[0] && cnt < d_size;
           index_input++, cnt++) {
        d[cnt] = double(in[index_input]);
      }
      printf("\naprès fill buffer cnt: %d\n",cnt);
      printf("\non consomme %d\n", index_input);
      consume_each(index_input);

      int r = 0;
      if (cnt==d_size) {
        printf("\non appelle mlfMainRx\n");

          /* Call the Rx library function */
        mlfMainRx(2, &outRxBits, &outcrcCheck, rx_in, mxNbBits);
        if (outRxBits != NULL){
          double *realdata,*realcrc;
          realdata = mxGetPr(outRxBits);
          r = mxGetN(outRxBits);
          printf("\nRX Bits:\n");
          for(int k=0;k<r;k++){
            printf("%1.0f",realdata[k]);
          }

          if(r==0){
            printf("RX packet not detected.");
          }
          else {
            realcrc = mxGetPr(outcrcCheck);
            // printf("\nCRC Check Len: %d\n",int(mxGetM(outcrcCheck)));

            if (*realcrc==0.0){
              printf("\nCRC not OK\n");
            }
            else if (*realcrc==1.0) {
              printf("\nCRC OK\n");
            }
            else printf("No packet detected.\n");

            for(int i=0;i < r; i++) {
              out[i] = realdata[i];
            }
          }
        }
        else {
          printf("Error, output NULL pointer.\n");
          throw new std::exception();
        } 
        for (int i = 0; i < Signal_len ; i++) {
          d[i] = d[i+Signal_len];
        }
        cnt = Signal_len;
      }

      return r;
    }

  } /* namespace ephyl */
} /* namespace gr */
