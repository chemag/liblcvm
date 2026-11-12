#!/usr/bin/env python3
"""Metadata-only fixtures for the sample table consistency checks.

Each file has one track (video unless noted) whose sample tables break, or
just meet, one of the rules liblcvm checks before expanding the tables into
per-frame lists (ISO/IEC 14496-12:2026):
* exactly one sample size box (stsz or stz2),
* stts total == stsz/stz2 sample_count,
* constant-size stsz: sample_count * sample_size <= file size (video only),
* ctts total <= sample_count,
* stss sample numbers in 1..sample_count, strictly increasing.
"""
import sys
sys.dont_write_bytecode = True
import struct
from pathlib import Path
import create_eac3_fixtures as m

ROOT = Path(__file__).resolve().parent

FRAMES = 4


def stts(count, delta=1000):
    return m.full('stts', m.u32(1) + m.u32(count) + m.u32(delta))


def stz2(count, field_size=8):
    if field_size == 4:
        entries = bytes([0x11]) * ((count + 1) // 2)
    elif field_size == 8:
        entries = bytes([1]) * count
    else:
        entries = m.u16(1) * count
    return m.full('stz2', bytes(3) + bytes([field_size]) + m.u32(count) +
                  entries)


def ctts(offsets):
    return m.full('ctts', m.u32(len(offsets)) + b''.join(
        m.u32(1) + struct.pack('>i', n) for n in offsets))


def stss(numbers):
    return m.full('stss', m.u32(len(numbers)) +
                  b''.join(m.u32(n) for n in numbers))


def trak(handler, entry, children, track_id=11, timescale=1000):
    tkhd = m.full('tkhd', m.u32(0) * 2 + m.u32(track_id) + m.u32(0) +
                  m.u32(4000) + bytes(16) + m.MATRIX +
                  m.u32(16 << 16) * 2, 3)
    mdhd = m.full('mdhd', m.u32(0) * 2 + m.u32(timescale) + m.u32(4000) +
                  bytes(4))
    hdlr = m.full('hdlr', m.u32(0) + handler.encode('ascii') + bytes(12) +
                  b'\0')
    stsd = m.full('stsd', m.u32(1) + entry)
    stbl = m.box('stbl', stsd + b''.join(children))
    return m.box('trak', tkhd + m.box('mdia', mdhd + hdlr +
                 m.box('minf', stbl)))


def video(*children):
    return trak('vide', m.VIDEO_ENTRY, children)


def audio(*children):
    return trak('soun', m.AAC, children, track_id=31, timescale=48000)


def write(name, tracks):
    data = m.box('ftyp', b'isom' + m.u32(0) + b'isom')
    data += m.box('moov', m.MVHD + b''.join(tracks))
    (ROOT / name).write_bytes(data)


def main():
    n = FRAMES
    keys = stss([1, 3])
    # valid: stsz table, stz2 (each field size), constant-size stsz that
    # fits in the file, a ctts shorter than the sample count (the last
    # offset applies to the remaining samples), and an audio track with a
    # constant-size stsz larger than the file (audio is not bounded)
    write('sample_tables_valid_stsz.mp4', [video(stts(n), m.stsz(n), keys)])
    for field_size in (4, 8, 16):
        write('sample_tables_valid_stz2_%i.mp4' % field_size,
              [video(stts(n), stz2(n, field_size), keys)])
    write('sample_tables_valid_constant_size.mp4',
          [video(stts(n), m.stsz(n, 1), keys)])
    write('sample_tables_valid_ctts_fewer.mp4',
          [video(stts(n), m.stsz(n), ctts([0, 2000]), keys)])
    write('sample_tables_valid_audio_constant_size.mp4',
          [video(stts(n), m.stsz(n), keys),
           audio(stts(12, 1024), m.stsz(12, 1 << 20))])
    # invalid: no sample size box, both of them, two stsz
    write('sample_tables_no_stsz.mp4', [video(stts(n), keys)])
    write('sample_tables_stsz_and_stz2.mp4',
          [video(stts(n), m.stsz(n), stz2(n), keys)])
    write('sample_tables_two_stsz.mp4',
          [video(stts(n), m.stsz(n), m.stsz(n), keys)])
    # invalid: the stts total differs from the sample count (video, audio)
    write('sample_tables_stts_more.mp4', [video(stts(n + 1), m.stsz(n), keys)])
    write('sample_tables_stts_fewer.mp4',
          [video(stts(n - 1), m.stsz(n), keys)])
    write('sample_tables_audio_stts_more.mp4',
          [video(stts(n), m.stsz(n), keys),
           audio(stts(12, 1024), m.stsz(11))])
    # invalid: a constant-size video stsz larger than the file
    write('sample_tables_constant_size_too_large.mp4',
          [video(stts(n), m.stsz(n, 1 << 20), keys)])
    # invalid: a ctts describing more samples than the track has
    write('sample_tables_ctts_more.mp4',
          [video(stts(n), m.stsz(n), ctts([0, 2000, 0, 0, 0]), keys)])
    # invalid: stss sample numbers out of range or out of order
    write('sample_tables_stss_zero.mp4', [video(stts(n), m.stsz(n),
                                                stss([0, 3]))])
    write('sample_tables_stss_past_end.mp4', [video(stts(n), m.stsz(n),
                                                    stss([1, n + 1]))])
    write('sample_tables_stss_not_increasing.mp4',
          [video(stts(n), m.stsz(n), stss([3, 1]))])
    write('sample_tables_stss_repeated.mp4',
          [video(stts(n), m.stsz(n), stss([1, 1]))])


if __name__ == '__main__':
    main()
