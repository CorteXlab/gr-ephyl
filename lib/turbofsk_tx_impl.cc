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
#include "turbofsk_tx_impl.h"
#include <algorithm>
#include "init_turbofsk.h"

// using namespace std;
// extern bool libturbofsk_init = true;

//int lib_turbofsk_init = 0;
//std::mutex init_mutex;

namespace gr {
  namespace ephyl {
    
    turbofsk_tx::sptr
    turbofsk_tx::make()
    {
      return gnuradio::get_initial_sptr
        (new turbofsk_tx_impl());
    }

    /*
     * The private constructor
     */
    turbofsk_tx_impl::turbofsk_tx_impl()
      : gr::block("TurboFSK TX",
              gr::io_signature::make(1, 1, sizeof(unsigned char)),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      // init_mutex.lock();
      // if (lib_turbofsk_init == 0) {
      //   if( !mclInitializeApplication(NULL,0) )
      //   {
      //     fprintf(stderr, "Could not initialize the application.\n");
      //     init_mutex.unlock();
      //     throw new std::exception();
      //   }

      //   if (!libTurboFSK_v2Initialize()){
      //     fprintf(stderr,"Could not initialize the library.\n");
      //     init_mutex.unlock();
      //     throw new std::exception();
      //     // printf("AAAA");
      //   } 
      // }
      // lib_turbofsk_init++;
      // init_mutex.unlock();
      get_turbofsk();
      
      // in EPHYL framework, the packet size is:
      // 14 payload chars + tab + slot_n char = 16 chars = 128 bits 
      NbBits = 128; 
      /* Create the input data */
      my_in = mxCreateDoubleMatrix(1,NbBits,mxREAL);

      b = mxGetPr(my_in);
      b_size = mxGetN(my_in);      

      /*  OUT = (64*32)+(1+(IN+16)/8)*4*137+(1+int((1+(IN+16)/8)*4/5))*137 */
      set_min_output_buffer(0,14652);
      // set_max_output_buffer(0,14652);
      // set_min_noutput_items(14652);
    }

    /*
     * Our virtual destructor.
     */
    turbofsk_tx_impl::~turbofsk_tx_impl()
    {
      mxDestroyArray(my_in);
      mxDestroyArray(outTx);
      // init_mutex.lock();
      // lib_turbofsk_init--;
      // if (lib_turbofsk_init == 0) {
      //   //  Call the library termination routine 
      //   libTurboFSK_v2Terminate();     

      //   /* Note that you should call mclTerminate application at the end of your application */
      //   mclTerminateApplication();
      // }
      // init_mutex.unlock();
      release_turbofsk();
    }

    void
    turbofsk_tx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = NbBits;
    }

    int
    turbofsk_tx_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {

      const unsigned char *in = (const unsigned char *) input_items[0];
      float *out = (float *) output_items[0];

      printf("\nTX Bits:\n");
      for(int k=0;k<b_size;k++){
        b[k] = double(in[k]);
        printf("%1.0f",b[k]);
      }

/************************************************************************/
      mlfMainTx(1, &outTx, my_in);
/************************************************************************/      
      // printf("\nOTHMANE\n");

      a = mxGetPr(outTx);
      a_size = mxGetM(outTx);  // We use mxGetM instead of mxGetN because mlfmainTx transposes input

      // printf("\nTX Signal:\n");
      // for(int k=0;k<a_size;k++){
      //   printf("%1.8f|",a[k]);
      // }

      // printf("\nTX Signal Size:\n");
      // printf("%d",(int)a_size);
      // printf("\n");

      //int min_out = std::min(noutput_items,int(a_size));
      //for(int i = 0; i < min_out; i++){

      for(int i=0;i < a_size; i++) {
        out[i] = a[i];
      }
      add_item_tag(0, nitems_written(0), pmt::string_to_symbol("packet_len"), pmt::from_long((int)a_size));
      consume_each (a_size);
      return a_size;

    }

  } /* namespace ephyl */
} /* namespace gr */

