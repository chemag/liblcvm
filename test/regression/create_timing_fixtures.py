#!/usr/bin/env python3
"""Metadata-only fixtures for shared timing parsing and derivation."""
import sys
sys.dont_write_bytecode = True
from pathlib import Path
import create_eac3_fixtures as m
import create_multivideo_fixtures as v

ROOT = Path(__file__).resolve().parent

def table(entries):
    return m.full('stts', m.u32(len(entries)) + b''.join(
        m.u32(count) + m.u32(delta) for count, delta in entries))

def write(name, tracks):
    data = m.box('ftyp', b'isom' + m.u32(0) + b'isom')
    data += m.box('moov', m.MVHD + b''.join(tracks))
    (ROOT / name).write_bytes(data)

write('timing_audio_stts.mp4', [
    v.video(11, 320, 240, 120, 30, 4),
    m.track(31, 'soun', m.AAC, 12288, 4000, 48000,
            sample_timing=table([(12, 1024)]), sample_sizes=m.stsz(12))])
write('timing_empty_ctts.mp4', [v.video(11, 320, 240, 4, 1, 4, ctts=[])])
write('timing_scalar_tick_overflow.mp4', [
    m.track(31, 'soun', m.AAC, 1, 1, 48000,
            sample_timing=table([(0xffffffff, 0xffffffff),
                                 (0xffffffff, 0xffffffff)]),
            # the largest sample count stsz can declare: the stts total
            # (2 * 0xffffffff samples) can never match it
            sample_sizes=m.stsz(0xffffffff, 1))])
