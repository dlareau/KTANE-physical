##
## This file is part of the libsigrokdecode project.
##
## Copyright (C) 2014 Matt Ranostay <mranostay@gmail.com>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, see <http://www.gnu.org/licenses/>.
##

import sigrokdecode as srd
from functools import reduce

byte_to_str = {0x86: "ACK",
               0x95: "NAK",
               0x82: "START",
               0x83: "END",
               0xD7: "WRITE",
               0xD2: "READ",
               0xB0: "NO_DATA",
               0xB1: "PING",
               0xC0: "STRIKE",
               0xC1: "SOLVE",
               0xC2: "CONFIG",
               0xC3: "READY",
               0xC4: "RESET",
               0xC5: "NUM_STRIKES",
               }

byte_to_str_small={0x86: "A",
                   0x95: "N",
                   0x82: "S",
                   0x83: "E",
                   0xD7: "W",
                   0xD2: "R",
                   0xB0: "ND",
                   0xB1: "P",
                   0xC0: "XXX",
                   0xC1: "YYY",
                   0xC2: "C",
                   0xC3: "G",
                   0xC4: "R",
                   0xC5: "#S",
                   }

def bytes_to_msgs(client_id, message_bytes):
    converted_bytes = []
    short_converted_bytes = []
    for byte in message_bytes:
        if(byte in byte_to_str):
            converted_bytes.append(byte_to_str[byte])
        else:
            converted_bytes.append(str(byte))
        if(byte in byte_to_str_small):
            short_converted_bytes.append(byte_to_str_small[byte])
        else:
            short_converted_bytes.append(str(byte))

    msg = str(client_id) + ":" + " ".join(converted_bytes)
    short_msg = str(client_id) + ":" + " ".join(short_converted_bytes)
    return [msg, short_msg]


class Decoder(srd.Decoder):
    api_version = 3
    id = 'dserial'
    name = 'DSerial'
    longname = 'DSerial'
    desc = 'A layer 3 protocol used for the Keep Taking and Nobody Explodes project by dlareau.'
    license = 'gplv2+'
    inputs = ['uart']
    outputs = ['dserial']
    options = (
        {'id': 'show_nodata', 'desc': 'Show "No Data"?', 'default': 'yes',
            'values': ('yes', 'no')},
    )
    annotations = (
        ('packet_ms', 'DSerial Packet Master'),
        ('packet_cl', 'DSerial Packet Client'),
        ('transaction', 'DSerial Transaction'),
    )
    annotation_rows = (
         ('master', 'Master', (0,)),
         ('clients', 'Clients', (1,)),
         ('transaction', 'Transaction', (2,)),
    )

    def __init__(self):
        self.reset()

    def reset(self):
        self.ss_pkt_ms, self.es_pkt_ms = 0, 0
        self.ss_pkt_cl, self.es_pkt_cl = 0, 0
        self.ss_trn, self.es_trn = 0, 0
        self.message_bytes_ms = []
        self.message_bytes_cl = []
        self.transaction_state = "WAITING"
        self.transaction_message = ""
        self.escape_next_ms = False
        self.escape_next_cl = False
        self.escape_parity_ms = 0
        self.escape_parity_cl = 0

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def putxms(self, data):
        self.put(self.ss_pkt_ms, self.es_pkt_ms, self.out_ann, data)

    def putxcl(self, data):
        self.put(self.ss_pkt_cl, self.es_pkt_cl, self.out_ann, data)

    def putxtrn(self, data):
        self.put(self.ss_trn, self.es_trn, self.out_ann, data)

    def decode(self, ss, es, data):
        ptype, rxtx, uart_data = data

        # Only care about data packets.
        if ptype != 'DATA':
            return

        datavalue, databits = uart_data

        if rxtx == 0:
            if len(self.message_bytes_ms) == 0:
                self.escape_parity_ms = 0
                self.ss_pkt_ms = ss
                if(datavalue != 0x82):
                    self.es_pkt_ms = es
                    self.putxms([0, ["BAD"]])
                    return
            if(datavalue == 0x9B):
                self.escape_next_ms = True
                self.escape_parity_ms = 1 - self.escape_parity_ms
                return
            if(self.escape_next_ms):
                self.escape_next_ms = False
                datavalue = datavalue + 0x80

            self.message_bytes_ms.append(datavalue)

            if datavalue != 0x83:
                return

            parity = reduce(lambda i, j: int(i) ^ int(j), self.message_bytes_ms)
            if(self.escape_parity_ms):
                parity = parity ^ 0x9B

            if(self.message_bytes_ms[0] != 0x82):
                msgs = ["ERR: NO START"]
            elif(self.message_bytes_ms[-1] != 0x83):
                msgs = ["ERR: NO END"]
            elif(parity & 0x7F != 0):
                msgs = ["ERR: BAD PARITY"]
            else:
                stripped_bytes = self.message_bytes_ms[2:-2]
                if(len(stripped_bytes) > 1 and stripped_bytes[1] == 0xC2):
                    config_data = stripped_bytes[2:]
                    ports = (config_data[0] >> 2) & 7
                    batteries = (config_data[0] >> 5) & 7
                    serial = "".join([chr(x) for x in config_data[1:6]])
                    serial += chr(ord('0') + ((config_data[6] >> 3) & 7))
                    indicators = (config_data[6] >> 6) & 7
                    config_str = "%d-%d-%d-%s" % (ports, batteries, indicators, serial)
                    msgs = ["X:WRITE CONFIG " + config_str, "W C " + config_str]
                else:
                    msgs = bytes_to_msgs(self.message_bytes_ms[1], stripped_bytes)

                # Transaction code
                if(self.transaction_state == "WAITING" or self.transaction_state == "MID-PING"):
                    self.ss_trn = self.ss_pkt_ms
                    if(stripped_bytes[0] == 0xD7):
                        self.transaction_state = "MID-WRITE"
                        self.transaction_message = msgs[0][8:]
                    elif(stripped_bytes[0] == 0xD2):
                        self.transaction_state = "MID-READ"
                    elif(stripped_bytes[0] == 0xB1):
                        self.transaction_state = "MID-PING"

                elif(self.transaction_state == "MID-READ2" and stripped_bytes[0] == 0x86):
                    self.transaction_state = "MID-READ3"

                else:
                    self.transaction_state = "WAITING"


            self.es_pkt_ms = es
            self.putxms([0, msgs])
            self.message_bytes_ms = []

        if rxtx == 1:
            if len(self.message_bytes_cl) == 0:
                self.escape_parity_cl = 0
                self.ss_pkt_cl = ss
                if(datavalue != 0x82):
                    self.es_pkt_cl = es
                    self.putxcl([0, ["BAD"]])
                    return

            if(datavalue == 0x9B):
                self.escape_next_cl = True
                self.escape_parity_cl = 1 - self.escape_parity_cl
                return
            if(self.escape_next_cl):
                self.escape_next_cl = False
                datavalue = datavalue + 0x80

            self.message_bytes_cl.append(datavalue)

            if datavalue != 0x83:
                return

            parity = reduce(lambda i, j: int(i) ^ int(j), self.message_bytes_cl)
            if(self.escape_parity_cl):
                parity = parity ^ 0x9B
            if(self.message_bytes_cl[0] != 0x82):
                msgs = ["ERR: NO START"]
            elif(self.message_bytes_cl[-1] != 0x83):
                msgs = ["ERR: NO END"]
            elif(parity & 0x7F != 0):
                msgs = ["ERR: BAD PARITY"]
            else:
                stripped_bytes = self.message_bytes_cl[2:-2]
                client_id = self.message_bytes_cl[1]
                msgs = bytes_to_msgs(client_id, stripped_bytes)

                # Transaction code
                if(self.transaction_state == "MID-WRITE" and stripped_bytes[0] == 0x86):
                    self.es_trn = es
                    self.putxtrn([2, ["M>%d: %s" % (client_id, self.transaction_message)]])
                    self.transaction_state = "WAITING"

                elif(self.transaction_state == "MID-PING" and stripped_bytes[0] == 0x86):
                    self.es_trn = es
                    self.putxtrn([2, ["CLIENT PINGED"]])
                    self.transaction_state = "WAITING"

                elif(self.transaction_state == "MID-READ"):
                    if(stripped_bytes[0] == 0x86):
                        self.es_trn = es
                        if(self.options["show_nodata"] == "yes"):
                            self.putxtrn([2, ["%d>M: NO DATA" % client_id]])
                        self.transaction_state = "WAITING"
                    else:
                        self.transaction_message = msgs[0][2:]
                        self.transaction_state = "MID-READ2"

                elif(self.transaction_state == "MID-READ3" and stripped_bytes[0] == 0x86):
                    self.es_trn = es
                    self.putxtrn([2, ["%d>M: %s" % (client_id, self.transaction_message)]])
                    self.transaction_state = "WAITING"

            self.es_pkt_cl = es
            self.putxcl([1, msgs])
            self.message_bytes_cl = []
