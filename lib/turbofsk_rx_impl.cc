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
#include <mutex>

// using namespace std;
// extern bool libturbofsk_init = true;

// std::mutex init_mutex_txrx;

namespace gr {
  namespace ephyl {
    
    turbofsk_rx::sptr
    turbofsk_rx::make(float Noise)
    {
      return gnuradio::get_initial_sptr
        (new turbofsk_rx_impl(Noise));
    }

    /*
     * The private constructor
     */
    turbofsk_rx_impl::turbofsk_rx_impl(float Noise)
      : gr::block("TurboFSK RX",
              gr::io_signature::make2(2, 2, sizeof(float), sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(unsigned char))),
        d_Noise(Noise)
    {
      get_turbofsk();

      // in EPHYL framework, the packet size is:
      // 14 payload chars + tab + slot_n char = 16 chars = 128 bits 
      NbBits = 128; 
      /*  Signal_len = (64*32)+(1+(NbBits+16)/8)*4*137+(1+int((1+(NbBits+16)/8)*4/5))*137 */
      Signal_len = 14652;      
      cnt = 0;
      r = 0, s = 0, t = 0;

      /* Create the input data */
      rx_in_I = mxCreateDoubleMatrix(1,2*Signal_len,mxREAL);
      rx_in_Q = mxCreateDoubleMatrix(1,2*Signal_len,mxREAL);

      // d = (float*)mxGetPr(rx_in_I);
      // d_size = mxGetN(rx_in_I);
      // e = (float*)mxGetPr(rx_in_Q);
      d = mxGetPr(rx_in_I);
      d_size = mxGetN(rx_in_I);
      e = mxGetPr(rx_in_Q);
      

      mxNbBits = mxCreateDoubleMatrix(1,1,mxREAL);
      double *bits = mxGetPr(mxNbBits);
      *bits = NbBits ;

      mxNoiseVar = mxCreateDoubleMatrix(1,1,mxREAL);
      double *NoiseVar = mxGetPr(mxNoiseVar);
      *NoiseVar = d_Noise ;

      // tmp = NULL;

      pkt_cnt = 0;

      set_min_output_buffer(0,NbBits);
    }

    /*
     * Our virtual destructor.
     */
    turbofsk_rx_impl::~turbofsk_rx_impl()
    {
      mxDestroyArray(rx_in_I);
      mxDestroyArray(rx_in_Q);
      mxDestroyArray(outRxBits);
      mxDestroyArray(outcrcCheck);
      mxDestroyArray(mxNbBits);
      mxDestroyArray(indexPayload);
      mxDestroyArray(mxNoiseVar);

      release_turbofsk();
    }


    int
    turbofsk_rx_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {

      const float *in_I = (const float *) input_items[0];
      const float *in_Q = (const float *) input_items[1];
      unsigned char *out = (unsigned char *) output_items[0];

      // printf("\nNINPUT 0: %d\n",ninput_items[0]);
      // printf("\nNINPUT 1: %d\n",ninput_items[1]);
      // printf("\navant fill buffer cnt: %d\n",cnt);

      int index_input;
      for (index_input=0;
           index_input < ninput_items[0] && cnt < d_size;
           index_input++, cnt++) {
        d[cnt] = (double)in_I[index_input];
        e[cnt] = (double)in_Q[index_input];
      }
      consume_each(index_input);

      printf("\naprès fill buffer cnt: %d\n",cnt);
      // printf("\non consomme %d\n", index_input);


      if (cnt==d_size) {
        double *realdata,*realcrc,*realindex;
        printf("\non appelle mlfMainRx\n");

          /* Call the Rx library function */
      /************************************************************************/
        init_mutex_txrx.lock();
        mlfMainRx(3, &outRxBits, &outcrcCheck, &indexPayload, rx_in_I, rx_in_Q, mxNbBits, mxNoiseVar);
        init_mutex_txrx.unlock();
      /************************************************************************/

        if (outRxBits != NULL){
          realdata = mxGetPr(outRxBits);
          r = mxGetN(outRxBits);
          printf("\nPacket: %d",pkt_cnt);
          pkt_cnt++;
          printf("\nRX Bits:\n");
          for(int k=0;k<r;k++){
            printf("%1.0f",realdata[k]);
          }

          if(r==0){
            printf("RX packet not detected.");
            s = mxGetN(indexPayload);
            if(s!=0){
              throw new std::exception();
            }
          }
          else {
            s = mxGetN(indexPayload);
            
            if(s!=0){
              // printf("\nOUT CRC DBG: %p\n",outcrcCheck);
              // printf("\nLEN CRC DBG: %d\n",int(mxGetN(outcrcCheck)));
              // printf("\nOUT INDEX DBG: %p\n",indexPayload);
              // printf("\nLEN INDEX DBG: %d\n",int(mxGetN(indexPayload)));
              realcrc = mxGetPr(outcrcCheck);
              realindex = mxGetPr(indexPayload);
              t = int(*realindex);

              printf("\nIndex: %d\n",t);
              // printf("\nNINPUT: %d\n",ninput_items[0]);

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
            else{
              // throw new std::exception();
              for(int i=0;i < r; i++) {
                out[i] = 0;
              }              
              printf("\nNINPUT: %d\n",ninput_items[0]);
            }
          }
        }
        else {
          printf("Error, output NULL pointer.\n");
          throw new std::exception();
        } 

        // for (int i = 0; i < Signal_len-t ; i++) {
        //   d[i] = d[i+t];
        // }
        // cnt = 0;
        for (int i = 0; i < Signal_len ; i++) {
          d[i] = d[i+Signal_len];
          e[i] = d[i+Signal_len];
        }
        cnt = Signal_len;
      }

      return r;
    }



    void
    turbofsk_rx_impl::setup_rpc()
    {
#ifdef GR_CTRLPORT
      add_rpc_variable(
        rpcbasic_sptr(new rpcbasic_register_get<turbofsk_rx, float>(
    alias(), "coefficient",
    &turbofsk_rx::Noise,
    pmt::mp(-1024.0f), pmt::mp(1024.0f), pmt::mp(0.0f),
    "", "Coefficient", RPC_PRIVLVL_MIN,
          DISPTIME | DISPOPTSTRIP)));

      add_rpc_variable(
        rpcbasic_sptr(new rpcbasic_register_set<turbofsk_rx, float>(
    alias(), "coefficient",
    &turbofsk_rx::set_Noise,
    pmt::mp(-1024.0f), pmt::mp(1024.0f), pmt::mp(0.0f),
    "", "Coefficient",
    RPC_PRIVLVL_MIN, DISPNULL)));
#endif /* GR_CTRLPORT */
    }

  } /* namespace ephyl */
} /* namespace gr */

