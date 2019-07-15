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
      cnt = 0;
      Signal_len = 14652;
      NbBits = 128;

      /* Create the input data */
      rx_in = mxCreateDoubleMatrix(1,Signal_len*2,mxREAL);    // Take input twice, to 
      d = mxGetPr(rx_in);
      d_size = mxGetN(rx_in);

      mxNbBits = mxCreateDoubleMatrix(1,1,mxREAL);
      realdata = mxGetPr(mxNbBits);
      *realdata = NbBits ;


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

    void
    turbofsk_rx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = Signal_len;
    }

    int
    turbofsk_rx_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {

      const float *in = (const float *) input_items[0];
      unsigned char *out = (unsigned char *) output_items[0];

      for(int k=0;k<ninput_items[0];k++){
        if (cnt != 0){
          d[k+ninput_items[0]] = d[k];  // Move elements by ninput_items[0]
        }
        d[k] = double(in[k]);   // Fill the emptied elements with new input
      }

      cnt += ninput_items[0];

      printf("\nCaptured Signal Size:\n");
      printf("%d",(int)cnt);
      printf("\nSignal Length :\n");
      printf("%d",(int)d_size);
      printf("\n");  

      if (cnt>=d_size) {
        cnt = 0;

          /* Call the Rx library function */
        mlfMainRx(2, &outRxBits, &outcrcCheck, rx_in, mxNbBits);

        if (outRxBits != NULL){
          realdata = mxGetPr(outRxBits);
          r = mxGetN(outRxBits);
          printf("\nRX Bits:\n");
          for(int k=0;k<r;k++){
            printf("%1.0f",realdata[k]);
          }
          if(r==0)
            printf("RX packet not detected.");
          else {
            printf("\nPayload Size:\n ");
            printf("%d",r);
            printf("\n");  

            NbErr = 0;
            // for(int k=0;k<Signal_len;k++){
            //   if(data[k] != realdata[k])
            //     NbErr++;
            // } 
            realcrc = mxGetPr(outcrcCheck);
            if (*realcrc==0.0){
              printf("CRC not OK\n");
            }
            else if (*realcrc==1.0) {
              printf("CRC OK\n");
            }
            else printf("No packet detected.\n");
          }
          for(int i=0;i < r; i++) {
            out[i] = realdata[i];
          }
        }
        else {
          printf("Error, output NULL pointer.\n");
          throw new std::exception();
        }

      }
      else {
        r = 0;
      }

      consume_each (r);
      return r;

    }

  } /* namespace ephyl */
} /* namespace gr */

