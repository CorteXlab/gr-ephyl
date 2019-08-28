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

from Crypto.Cipher import AES
import base64


class msg_mux(gr.sync_block):
    """

    Concatenates message data coming from inputs (payload, frame & slot number)
    """
    def __init__(self):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Concatenate Messages',   # will show up in GRC
            in_sig=[],
            out_sig=[]
        )
        self.message_port_register_in(pmt.to_pmt("data"))        
        self.set_msg_handler(pmt.intern("data"), self.handle_data)
        self.message_port_register_in(pmt.to_pmt("slot_n"))
        self.set_msg_handler(pmt.intern("slot_n"), self.handle_slot)
        self.message_port_register_in(pmt.to_pmt("frame_n"))
        self.set_msg_handler(pmt.intern("frame_n"), self.handle_frame)        
        
        self.message_port_register_out(pmt.to_pmt("final_msg"))

        self.lock = threading.Lock()

        self.slot_n = -1
        self.frame_n = 0
        self.frame_msg = 0
        self.data = []


    def decrypt(self,mystr) :
        secret_key = '0123456789ABCDEF' # create new & store somewhere safe
        cipher = AES.new(secret_key,AES.MODE_ECB)
        try :
            decoded = cipher.decrypt(base64.b64decode(mystr))
            return decoded.strip()  # Get rid of space padding
        except :
            return False

    def handle_frame(self, msg_pmt):
        with self.lock :

            self.frame_n = pmt.to_python(pmt.cdr(msg_pmt))
            self.frame_msg = np.uint8([ord(c) for c in (str(self.frame_n[1]+1))+"\t"])
            # print self.frame_msg
    

    def handle_slot(self, msg_pmt):
        with self.lock :

            self.slot_n = pmt.to_python(pmt.cdr(msg_pmt))
            self.slot_msg = np.uint8([ord(c) for c in (str(self.slot_n[1])+"\t")])
            # print self.slot_n[1]

    def handle_data(self, msg_pmt):
        with self.lock : 
            self.data = pmt.to_python(pmt.cdr(msg_pmt))
            # self.data = np.delete(self.data,-1)     # Delete \n
            l = [chr(c) for c in self.data]
            l = ''.join(l)
            # ID = self.decrypt(l[:24])
            # # print ID
            
            # # print "Successful decryption of ID : " + ID
            # l = ID + "\t" + l[24:]
            l = list(l)
            self.data = [ord(c) for c in l]     # We replace the crypted part with the decrypted one
            ID = False
            # print self.data
            if self.slot_n >= 0 :
                res = np.append(self.slot_msg, self.data)
                res = np.append(self.frame_msg, res)
                res = res.tolist()
                # print res
                res_pdu = pmt.cons(pmt.make_dict(), pmt.init_u8vector(len(res),res))
                self.message_port_pub(pmt.to_pmt("final_msg"), res_pdu)


            # if any([ID]) :      # Check if Crypted ID is valid
            #     # print "Successful decryption of ID : " + ID
            #     l = ID + "\t" + l[24:]
            #     l = list(l)
            #     self.data = [ord(c) for c in l]     # We replace the crypted part with the decrypted one
            #     ID = False
            #     if self.slot_n >= 0 :
            #         res = np.append(self.slot_msg, self.data)
            #         res = np.append(self.frame_msg, res)
            #         res = res.tolist()
            #         # print res
            #         res_pdu = pmt.cons(pmt.make_dict(), pmt.init_u8vector(len(res),res))
            #         self.message_port_pub(pmt.to_pmt("final_msg"), res_pdu)                    

