#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2019 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import numpy as np
from gnuradio import gr
import time
import threading
import pmt

IDLE = 0
BCH = 1
SYNC = 2
PUSCH = 3
GUARD = 4
PROC = 5


class bs_scheduler(gr.sync_block):
    """EPHYL Demo : Base Station 
    - All durations are expressed in millisecond
    """
    def __init__(self, num_slots=5,
        bch_time=20, Sync_time=50, guard_time=100, Slot_time=50, Proc_time = 50, 
        Beacon_Sequence=[0],sample_rate=200000):
        gr.sync_block.__init__(self,
            name="BS Scheduler",
            in_sig=[np.complex64],
            out_sig=[np.complex64,np.complex64])


        # self.message_port_register_out(pmt.intern("slot_n"))
        ##################################################
        # Parameters
        ##################################################
        # self.T_bch = T_bch        
        # self.T_g = guard_time
        # self.T_rf = RF_switch_time
        # self.T_s = Slot_duration
        self.num_slots = num_slots
        self.bcn = Beacon_Sequence
        self.samp_rate = int(sample_rate/1000)
        # self.samp_rate = int(sample_rate/1000)

        ##################################################
        # Variables
        ##################################################
        # self.state = -1
        self.state = IDLE  # DEBUG (no uhd)
        self.state_dbg = -1
        self.slot_cnt_dbg = -1
        self.slot_cnt = -1
        self.rx_time = 0
        self.timer = 0
        self.samp_cnt = 0
        self.samp_cnt_abs = 0
        self.to_return1 = 0
        self.bcn_sent = False
        self.frame_cnt = 0

        self.diff = self.left = 0

        ## Here we set states data, 
        ## PS : SYNC has a constant offset of +guard_time to compensate the LISTEN state of the sensor nodes
        ## Have a look at the same variable in the sensor scheduler block
        self.STATES = [range(6) \
            ,['IDLE','BCH','SYNC','PUSCH','GUARD','PROC'] \
            ,[0,bch_time,Sync_time,Slot_time,guard_time,Proc_time]]

        # self.frame_time = self.T_bch + self.T_rf + num_slots*(self.T_s+self.T_g) + self.T_rf

        self.lock = threading.Lock()  

    def to_time(self,n_samp) :
        return n_samp/float(self.samp_rate)

    def to_samples(self,duration) :
        return int(duration*self.samp_rate)

    def next_state(self) :
        # state = self.STATES[0][int((state+1)%len(self.STATES[0]))]
        if self.state < len(self.STATES[0])-1 :
            self.state += 1
        else :
            self.state = 0

    def run_state(self,Input,output1,output2) :

        state_samp = self.to_samples(self.STATES[2][self.state])
        self.diff = state_samp-self.samp_cnt

        ###############################################################################
        ## If the cuurent state cannot run completely, 
        ## i.e the sample count exceeds the number of samples required for the current state      
        if self.diff < 0 :

            self.samp_cnt_abs += self.diff
            self.samp_cnt = 0
            output1 = np.delete(output1,slice(len(output1)+self.diff,len(output1)))    # Since diff is negative
            
            # if self.state == BCH :
            #     self.samp_cnt_abs = 0

            if self.state == SYNC :
                self.slot_cnt += 1
                # print "BS : " + str(self.nitems_written(0)+len(output1))

            elif self.state == GUARD :
                self.slot_cnt += 1
                if self.slot_cnt < self.num_slots :
                    # Return to PUSCH
                    self.state -= 2

                else :
                    # print "[BS] TOTAL SLOTS + GRD TIME : " + str(self.to_time(self.samp_cnt_abs))
                    self.slot_cnt = -1

            elif self.state not in self.STATES[0] :
                print("STATE ERROR")
                exit(1)
            
            elif self.state == PUSCH : 
                output1[:] = Input[:len(output1)]
            ## DEBUG    
            # elif self.state in (SYNC,BCH) :
            # #     output1[:] = [1]*len(output1)
            #     output1[:] = Input[:len(output1)]
            else :
                output1[:] = [0]*len(output1)

            output2[:] = [0]*len(output2)
            self.next_state()
            
            # Add tags for each state
            offset = self.nitems_written(0)+len(output1)
            if self.state == PROC :
                key = pmt.intern("FRAME")
                value = pmt.to_pmt(self.frame_cnt)
                # value = pmt.to_pmt(self.frame_cnt-1)
                print "[BS] ================= FRAME " + str(self.frame_cnt) + " FINISH ================="
                self.frame_cnt += 1
                self.samp_cnt_abs = 0
                self.bcn_sent = False
            else :
                key = pmt.intern(self.STATES[1][self.state])
                value = pmt.to_pmt(self.slot_cnt)
            self.add_item_tag(0,offset, key, value)


            # if self.frame_cnt == 5 :
            #     stop()

        ###############################################################################
        ## If the cuurent state can still run completely one more time
        else :
            self.samp_cnt -= len(output1)

            if self.state == PUSCH :
                output1[:] = Input[:]

            elif self.state == BCH :
                if not(self.bcn_sent) :
                    bcn_z = self.bcn        # Some zero padding
                    max_output = min(len(output1), len(bcn_z))
                    output1 = output1[:max_output]
                    # Q = int(len(output1)/max_output)
                    # R = int(len(output1)%max_output)
                    # output1[:] = bcn_z*Q + [0]*R
                    output1[:] = bcn_z[:max_output]
                    self.bcn_sent = True
                else : 
                    output1[:] = [0]*len(output1)
                    output2[:] = [0]*len(output2)                    

            elif self.state == SYNC :
                output1[:] = [0]*len(output1)
                output2[:] = [0]*len(output2)
            else :
                output1[:] = [0]*len(output1)
                # output2[:] = [0]*len(output2)

            self.samp_cnt += len(output1)
        ###############################################################################

        if self.state != BCH :
            output1[:] = Input[:len(output1)]

        self.to_return1 = len(output1)
        # output2 = output2[:len(output1)]
        # output2[:] = [0]*len(output1)

    def work(self, input_items, output_items):
        with self.lock :

            self.samp_cnt += len(output_items[0])
            self.samp_cnt_abs += len(output_items[0])

            if self.state == -1 :
                num_input_items = len(input_items[0])
                nread = self.nitems_read(0)
                tags = self.get_tags_in_range(0, nread, nread+num_input_items)
                for tag in tags:
                    msg = pmt.cons(tag.key,tag.value)
                    msg_tup = pmt.to_python(msg)
                    if msg_tup[0] == 'rx_time' :
                        # self.rx_time = msg_tup[1][1]
                        self.rx_time = msg_tup[1][0]+msg_tup[1][1]
                        print "[BS] RX TIME : " + str(self.rx_time)
                        self.samp_cnt -= self.rx_time
                        self.samp_cnt_abs -= self.rx_time
                        self.state = IDLE
                        break
                return len(output_items[0])

            # if self.state_dbg != self.state :
            #     self.state_dbg = self.state
            #     print "[BS] STATE " + self.STATES[1][self.state] + " START : " + str(self.to_time(self.to_return1))
            #     print "[BS] STATE " + self.STATES[1][self.state] + " START : " + str(self.to_time(self.samp_cnt_abs))
                
                # if (self.state == PUSCH) :
                #     print "[BS] STATE PUSCH @ SLOT : " + str(self.slot_cnt)
                # self.samp_cnt = 0
            # else :

            if self.state == BCH :
                self.run_state(input_items[0],output_items[1],output_items[0])
            else :
                self.run_state(input_items[0],output_items[0],output_items[1])

            return self.to_return1

