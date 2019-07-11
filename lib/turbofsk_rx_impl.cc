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
      // init_mutex_rx.lock();
      // if (lib_turbofsk_init_rx == 0) {
      //   if( !mclInitializeApplication(NULL,0) )
      //   {
      //     fprintf(stderr, "Could not initialize the application.\n");
      //     init_mutex_rx.unlock();
      //     throw new std::exception();
      //   }

      //   if (!libTurboFSK_v2Initialize()){
      //     fprintf(stderr,"Could not initialize the library.\n");
      //     init_mutex_rx.unlock();
      //     throw new std::exception();
      //   } 
      // }
      // lib_turbofsk_init_rx++;
      // init_mutex_rx.unlock();
      get_turbofsk();
      
      NbBits = 5473;

      /* Create the input data */
      my_in = mxCreateDoubleMatrix(1,NbBits,mxREAL);
      b = mxGetPr(my_in);
      b_size = mxGetN(my_in);      

      mxNbBits = mxCreateDoubleMatrix(1,1,mxREAL);
      realdata = mxGetPr(mxNbBits);
      *realdata = (double) NbBits;


      set_min_output_buffer(0,16);
    }

    /*
     * Our virtual destructor.
     */
    turbofsk_rx_impl::~turbofsk_rx_impl()
    {
      mxDestroyArray(my_in);
      mxDestroyArray(outRxBits);
      mxDestroyArray(outcrcCheck);

      // init_mutex_rx.lock();
      // lib_turbofsk_init_rx--;
      // if (lib_turbofsk_init_rx == 0) {
      //   libTurboFSK_v2Terminate();     
      //   mclTerminateApplication();
      // }
      // init_mutex_rx.unlock();
      release_turbofsk();
    }

    void
    turbofsk_rx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = 16;
    }

    int
    turbofsk_rx_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {

      const float *in = (const float *) input_items[0];
      unsigned char *out = (unsigned char *) output_items[0];


      for(int k=0;k<b_size;k++){
        b[k] = double(in[k]);
      }

        /* Call the Rx library function */
      mlfMainRx(2, &outRxBits, &outcrcCheck, my_in, mxNbBits);
      if (outRxBits != NULL){
        realdata = mxGetPr(outRxBits);
        r = mxGetN(outRxBits);
        printf("RX:");
        for(int k=0;k<r;k++){
          printf("%1.0f",realdata[k]);
        }
        printf("\n");
        // if(r==0)
        //   printf("RX packet not detected.");
        // else {
        //   NbErr = 0;
        //   // for(int k=0;k<NbBits;k++){
        //   //   if(data[k] != realdata[k])
        //   //     NbErr++;
        //   // }
        //   realdata = mxGetPr(outcrcCheck);
        //   if (*realdata==0.0){
        //     printf("CRC not OK %d Errors / %d bits\n",NbErr,NbBits);
        //   }
        //   else if (*realdata==1.0) {
        //     printf("CRC OK     %d Errors / %d bits\n",NbErr,NbBits);
        //   }
        //   else printf("No packet detected.\n");
        // }
        }
      else {
        printf("Error, output NULL pointer.\n");

      }


      consume_each (a_size);
      return a_size;

    }

  } /* namespace ephyl */
} /* namespace gr */

