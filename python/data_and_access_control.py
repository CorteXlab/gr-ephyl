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
import pmt
import time
import random
import threading
from gnuradio import gr, gr_unittest, blocks

import ntpath
from Crypto.Cipher import AES
import base64

import string
import re

class data_and_access_control(gr.sync_block):
    """
    docstring for block data_and_access_control
    """
    def __init__(self, bs_slots,Control):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Data & Slot Control',   # will show up in GRC
            in_sig=[],
            out_sig=[]
        )
        
        self.message_port_register_out(pmt.to_pmt("Array"))
        self.message_port_register_out(pmt.to_pmt("Data"))
        self.message_port_register_out(pmt.to_pmt("PER"))


        self.message_port_register_in(pmt.intern("busy"))
        self.set_msg_handler(pmt.intern("busy"), self.handle_busy)
        self.message_port_register_in(pmt.intern("DL"))
        self.set_msg_handler(pmt.intern("DL"), self.handle_DL)        

        self.lock = threading.Lock()
        self.slot_n = -1
        self.data = []
        self.busy = True
        self.lines = []    
        self.i = 0
        self.DL = ''

        self.frame = '0'
        self.control = Control

        self.ID = random.choice(string.ascii_letters)

        self.bs_slots = bs_slots
        self.sn_slots = []

        self.RX = ''
        self.RX_frame = []

        self.dl_en = False

        self.PER = 64*[0]

        self.error_list = []
        self.cnt = 0
        self.error = 0
        self.cnt_dbg = 0

        self.watch = 0

        # Encrypt ID
        # self.enc_ID = self.encrypt(self.ID)
        self.enc_ID = self.ID

        self.tmp_data = self.rand_data(14)      # Created to keep the same data bits
        self.lines = self.gen_rand_pld(self.tmp_data,True,2)

        result = self.compare_pld(self.lines,self.lines)
        # Generate new payload 
        self.lines = self.gen_rand_pld(self.tmp_data,False,2,result[1])

    def rand_slots(self,len) :
        res = [random.choice(self.bs_slots) for _ in xrange(len)]
        return map(str, res)

    def rand_data(self,len) :
        res = ''
        letters = string.ascii_lowercase
        res =  ''.join(random.choice(letters) for i in xrange(len))
        return res 

    # def encrypt(self,mystr) :
    #     secret_key = '0123456789ABCDEF' # create new & store somewhere safe
    #     cipher = AES.new(secret_key,AES.MODE_ECB) 
    #     try :
    #         msg_text = mystr.rjust(16)
    #         encoded = base64.b64encode(cipher.encrypt(msg_text))
    #         return encoded
    #     except :
    #         print "Encryption Error, input must be multiple of 16"


    # Generate random payload
    def gen_rand_pld(self,data=False,rand_s=True,n=2,slots=[]) :    
            res = []
            slots = map(str, slots)
            if not data :
                data = self.rand_data(14)    
            
            if rand_s :  
                slots = self.rand_slots(n)
                for j in xrange(n):
                    res.append(slots[j]+'\t'+data)
            else :
                if any(slots) :
                    for j in range(len(slots)):
                        # Small note here, the payload is adapted if the slot number contains more than two characters
                        res.append(slots[j]+'\t'+data[:len(data)-len(slots[j])+1])      
                else :
                    res = '0'+'\t'+data
            return res 


    # Compare Tx & Rx PLD
    def compare_pld(self,TX,rx) :    
            v=''
            h = -1
            active_slots = []
            used_slots = []
            new_slots = []
            remaining = []
            self.error = 0
            tx = TX

            # Verify that rx and tx frames are arrays, to avoid errors when sweeping
            try :
                TX[0][0]
            except :
                tx = [TX]
            try :
                rx[0][0]
            except :
                rx = [rx]
            try :
                np.shape(rx)[1]
            except :
                rx = np.array([rx])
            ############################################################################################
            # print rx
            for f in range(len(rx)) :
                if len(rx[f])>3 and rx[f][1].isdigit() :

                    active_slots = np.append(active_slots,rx[f][1])
                    for j in xrange(len(tx)):
                        tx[j] = re.split(r'\t+', tx[j])
                        used_slots = np.append(used_slots,tx[j][0])
                        # Check for slot activity
                        if rx[f][1] == tx[j][0]:     
                            v += 's'
                            # Check for matching id
                            if rx[f][2] == self.ID:     
                                v += 'i'
                                # Check for matching payload
                                if rx[f][3] == tx[j][1]:     
                                    v += 'p'
                                h = f 

                        rx[f][2] == self.ID
                        tx[j] = '\t'.join(tx[j])

            if not (any(active_slots) and any(used_slots))  :
                active_slots = used_slots = [0]            
            ############################################################################################
            used_slots = list(dict.fromkeys(used_slots))    # Remove duplicates
            remaining = list(set(map(str, self.bs_slots)) - set(active_slots))
            remaining.sort()

            #################################################################
            # Use all slots
            if self.control == 'all' :
                new_slots = self.bs_slots   
            #################################################################
            elif self.control == 'random' :
                new_slots = np.random.choice(self.bs_slots, 2).tolist()
            #################################################################
            # Increment each frame
            elif self.control == 'increment' :
                new_slots = [int(used_slots[0])]
                if new_slots[0]+1 not in self.bs_slots :
                    new_slots = [0]
                else:
                    new_slots = [int(used_slots[0]) + 1]

            #################################################################                
            elif self.control == 'basic' :
                '''
                With Othmane basic Control Policy:
                If success, keep one of the good usedslots
                If failure, find remaining unused slots, if none choose 2 random 
                '''
                if v.count('p') > 0 :
                    new_slots = rx[h][1]
                else :
                    if remaining :
                        new_slots = np.random.choice(remaining, min(2,len(remaining))).tolist()
                    else :
                        new_slots = np.random.choice(self.bs_slots, 2).tolist()               
            #################################################################   
            # With UCB
            elif self.control == 'ucb' :
                pass










            #################################################################
            # With No Control Policy, keep old slots
            else :
                self.control == 'NONE'
                new_slots = used_slots
                
            ############################################################################################
            # print "[SN "+self.ID+"] Used Slots " + str(used_slots) + "\n"
            # print "[SN "+self.ID+"] Active Slots " + str(active_slots) + "\n"
            # print "[SN "+self.ID+"] Remaining Slots " + str(remaining) + "\n"
            # print "[SN "+self.ID+"] New Slots " + str(new_slots) + "\n"

            if v.count('p') > 0 :
                # if v.count('s') > 0 :
                self.error = 0
                # else :
                #     self.error = 1
            else :
                self.error = 1


            used_slots = list(set(used_slots))
            used_slots.sort()
            active_slots = list(set(active_slots))
            active_slots.sort()
            new_slots = list(set(new_slots))
            new_slots.sort()

            return [v,new_slots,active_slots,self.error]


    def handle_busy(self, msg_pmt):
        with self.lock :        
            self.busy = pmt.to_python(msg_pmt)

            if self.busy != True :
                if self.i < len(self.lines) :
                    # Scheduler informs a reset before sending data
                    if self.busy == 'RESET' :
                        self.i = 0
                        print "[SN "+self.ID+"] ACCESS POLICY: " + self.control + "\n"

                    # Scheduler informs a frame reset (frame finished)
                    elif self.busy == 'RESET_FRAME' :
                        ##################### PROCESS RECEIVED FRAMES AND COMPUTE PER ##############################
                        # Activity in DL :
                        if any(self.RX_frame) :
                            # Check if valid + if multiple or single received frame
                            if len(self.RX_frame) >= 4 and len(self.RX_frame)%4 == 0 :
                                self.RX_frame = np.reshape(self.RX_frame, (-1, 4))      # Sort received frames by rows

                                # delete overlapping frames = delete array m on axis 0 (array,index,axis)
                                tmp = self.RX_frame
                                for m in range(len(self.RX_frame)) :
                                    if self.RX_frame[m][0] != self.RX[0]:
                                        tmp = np.delete(self.RX_frame, m, 0)    # delete overlapping erroneous frames = delete array m on axis 0 (array,index,axis)  
                                self.RX_frame = tmp

                                result = self.compare_pld(self.lines,self.RX_frame)

                                # Generate new payload 
                                self.lines = self.gen_rand_pld(self.tmp_data,False,2,result[1])
                                
                                print "[SN "+self.ID+"] Score of Frame : " + str(result[0]) + "\n"
                        ###########################################################################################
                        # No activity in DL
                        else:
                            # print self.lines
                            result = self.compare_pld(self.lines,self.lines)
                            # print self.lines
                            # Generate new payload 
                            self.lines = self.gen_rand_pld(self.tmp_data,False,2,result[1])
                            print "[SN "+self.ID+"] Score of unknown Frame" + " : " + "\n"
                            self.error = 1
                        ############################################################################################
                        self.RX_frame = [] 
                        self.error_list = np.append(self.error_list,self.error)
                        # print "PER counter = " + str(self.cnt)
                        # Compute self.PER:
                        if self.cnt%6==0 and self.cnt !=0 :
                            # Shift PER to the right
                            self.PER = [0] + self.PER[:-1]
                            self.PER[0] = (self.PER[4] + self.PER[3] + self.PER[2] + self.PER[1] + sum(self.error_list)/float(self.cnt))/5
                            per_pdu = pmt.cons(pmt.make_dict(), pmt.init_f32vector(64,self.PER))    
                            self.cnt=0
                            self.error_list = []
                            self.message_port_pub(pmt.to_pmt("PER"), per_pdu) 
                        self.cnt += 1   # Frame counter mod N (where N is averaging size)
                        
                    ########################################################################################################
                    else :
                    # Scheduler requests node ID and payload array to compute IQ signal length
                        if self.busy == 'ARRAY' :
                            # Add ID for scheduler, removed also later by the scheduler
                            tmp = self.ID + self.lines[self.i]
                            self.message_port_pub(pmt.to_pmt("Array"), pmt.to_pmt(tmp))   # Send 1st char of each line (aka slots)
                    ########################################################################################################                            
                    # Scheduler requests payload array to be sent in PHY chain
                        elif self.busy == 'DATA' :
                            # Data is (node_id + line_i)
                            data = self.enc_ID + '\t' + self.lines[self.i][2:]     # Remove the first char and tabulation
                            OUT = pmt.cons(pmt.make_dict(), pmt.init_u8vector(len(data),[ord(c) for c in data]))    # Data = encrypted node_id + line_i
                            self.message_port_pub(pmt.to_pmt("Data"), OUT) 
                        self.i += 1
                else :
                    self.message_port_pub(pmt.to_pmt("Array"), pmt.to_pmt("STOP"))
                    self.i = 0 

            self.busy = True


    # Here we process all DL data broadcasted by the BS
    def handle_DL(self, msg_pmt):
        with self.lock :        

            self.watch += 1
            self.DL = pmt.to_python(msg_pmt)
            # print self.DL[1]
            # Look for a tab caracter in DL message, to avoid processing beacon message
            if ord('\t') in self.DL[1] :
                result = [0]
                
                l = [chr(c) for c in self.DL[1]]
                tab_pos = [pos for pos, char in enumerate(l) if char == '\t']     # \t is the separator
                l = ''.join(l)
                self.RX = re.split(r'\t+', l)

                # Correct a silly bug where a '0' is converted to '\x00', not the optimal correction
                if '\x00' in self.RX[0] :   
                    self.RX = ['0'] + [t.replace('\x00', '') for t in self.RX]

                # If received frame is valid <> 4 fields separated with a \t
                if len(self.RX)%4 == 0 :
                    self.RX_frame = np.append([self.RX_frame],[self.RX])

            self.DL = '' 