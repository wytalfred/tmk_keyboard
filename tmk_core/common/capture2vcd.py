#!/usr/bin/python3
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# This converts pin capture data into VCD format
#
# Capture record format: <time> ':' <stat>
#     time   6 hex chars; timestamp of event, which counts 1/16 us per tick
#           and is comprised of three bytes:
#               timer xtra(overflow count)
#               timer high
#               timer low
#     stat   2 hex chars;  Pin state
#

import sys
import re

# https://github.com/westerndigitalcorporation/pyvcd
from vcd import VCDWriter

if len(sys.argv) > 2:
    print(f"Usage: {sys.argv[0]} [infile]")
    sys.exit(1)

if len(sys.argv) == 2:
    file = open(sys.argv[1])
else:
    file = sys.stdin

# write VCD
from datetime import datetime
with VCDWriter(sys.stdout, timescale='1 us', date=datetime.utcnow().ctime()) as writer:

    # prams: scope, name, var_type, size
    port = writer.register_var('tmk_capture', 'port', 'wire', size=8, init=0xFF)

    p = re.compile(r'([0-9A-Fa-f]{6}):([0-9A-Fa-f]{2})')

    ext_time = 0
    prv_time = 0
    prv_stat = 0
    for line in file:
        for record in line.strip().split():
            #if len(record) == 9:
            m = p.match(record)
            if m is not None:
                time = int(record[0:6], 16)
                stat = int(record[7:9], 16)
                #xtra = int(record[7:9], 16)

                # time rollover record
                if prv_stat == stat and (time & 0xff0000) == 0:
                    ext_time += 1
                else:
                    # adhoc: timer is overflowed during pin_change ISR but xtra is not updated yet by timer ISR
                    if (prv_time & 0xffff) > (time & 0xffff):
                        if (prv_time >> 16) == (time >> 16):
                            print('time flipped: ', hex(prv_time), '>', hex(time), file=sys.stderr);
                            time += 0x10000

                    # time rollover
                    if prv_time > time:
                        ext_time += 1

                prv_time = time & 0xffffff;
                prv_stat = stat

                # time: 1/16 us(62.5 ns) per tick(@16MHz+Prescaler:1)
                writer.change(port, ((ext_time * 0x1000000) + time + 8)/16, stat)
            else:
                print('Invalid record: ', record, file=sys.stderr);
                #sys.exit(1)
