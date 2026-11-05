#!/usr/bin/env python3
"""Small deterministic timestamp arithmetic fixtures (no encoded media)."""
import sys
sys.dont_write_bytecode = True
import struct
from pathlib import Path
import create_eac3_fixtures as m

ROOT = Path(__file__).resolve().parent

def write(name, count, delta, offsets=None, version=1, hz=1000):
    stsd = m.full('stsd', m.u32(1) + m.VIDEO_ENTRY)
    stts = m.full('stts', m.u32(1) + m.u32(count) + m.u32(delta))
    ctts = b''
    if offsets is not None:
        ctts = m.full('ctts', m.u32(len(offsets)) + b''.join(
            m.u32(1) + struct.pack('>i', offset) for offset in offsets), version << 24)
    stss = m.full('stss', m.u32(1) + m.u32(1))
    tkhd = m.full('tkhd', m.u32(0)*2 + m.u32(11) + m.u32(0) + m.u32(4000)
                  + bytes(16) + m.MATRIX + m.u32(16 << 16)*2, 3)
    mdhd = m.full('mdhd', m.u32(0)*2 + m.u32(hz) + m.u32(4000) + bytes(4))
    hdlr = m.full('hdlr', m.u32(0) + b'vide' + bytes(12) + b'\0')
    trak = m.box('trak', tkhd + m.box('mdia', mdhd + hdlr + m.box('minf',
                          m.box('stbl', stsd + stts + ctts + stss))))
    (ROOT / name).write_bytes(m.box('ftyp', b'isom' + m.u32(0) + b'isom')
                             + m.box('moov', m.MVHD + trak))

write('arithmetic_multiple_wraps.mp4', 5, 3000000000)
write('arithmetic_large_average.mp4', 5, 3000000000, hz=1)
write('arithmetic_exact_boundaries.mp4', 4, 2147483648, hz=1)
write('arithmetic_signed_pts.mp4', 4, 2000000000, [-100,1000000000,-1000000000,0])
write('arithmetic_reordered_pts.mp4', 4, 2000000000, [0,2100000000,-1000000000,0])
for version in (0,1):
    write('arithmetic_signed_ctts_v'+str(version)+'.mp4', 4, 1000,
          [-100,20,-20,0], version)
