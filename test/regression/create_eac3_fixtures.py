#!/usr/bin/env python3
"""Generate tiny metadata-only regression MP4s; no media is decoded or rewritten.

The video descriptor is synthetic. Audio descriptors exercise ETSI TS 102 366
Annex F and Dolby's optional EC3 extension. These are parser fixtures, not
playable capture recordings or evidence of a decoder/pipeline result.
"""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent

def u16(x):
    return struct.pack('>H', x)

def u32(x):
    return struct.pack('>I', x)

def box(name, data):
    return u32(8 + len(data)) + name.encode('ascii') + data

def full(name, data, flags=0):
    return box(name, u32(flags) + data)

MATRIX = b''.join(u32(x) for x in (0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x40000000))
MVHD = full('mvhd', u32(0) * 2 + u32(1000) + u32(4000) + u32(0x10000) +
            u16(0x100) + bytes(10) + MATRIX + bytes(24) + u32(4))
VIDEO_HEADER = (bytes(6) + u16(1) + bytes(16) + u16(16) * 2 +
                u32(0x480000) * 2 + u32(0) + u16(1) + bytes(32) +
                u16(24) + u16(0xffff))
# A structurally valid AVCDecoderConfigurationRecord with no SPS/PPS.
# liblcvm correctly leaves bitstream-derived color/profile fields unknown.
VIDEO_ENTRY = box('avc1', VIDEO_HEADER + box('avcC', bytes.fromhex('0142001effe000')))
STTS = full('stts', u32(1) + u32(120) + u32(1000))


def track(track_id, handler, entry, mdhd_duration, tkhd_duration, timescale,
          sample_timing=None):
    tkhd = full('tkhd', u32(0) * 2 + u32(track_id) + u32(0) +
                u32(tkhd_duration) + bytes(8) + bytes(8) + MATRIX +
                u32(16 << 16) * 2, 3)
    mdhd = full('mdhd', u32(0) * 2 + u32(timescale) + u32(mdhd_duration) + bytes(4))
    hdlr = full('hdlr', u32(0) + handler.encode('ascii') + bytes(12) + b'\0')
    stsd = full('stsd', u32(1) + entry)
    timing = sample_timing if sample_timing is not None else (
        STTS if handler == 'vide' else b'')
    stbl = box('stbl', stsd + timing)
    return box('trak', tkhd + box('mdia', mdhd + hdlr + box('minf', stbl)))


def audio_header(channels=2, rate=48000):
    return bytes(6) + u16(1) + bytes(8) + u16(channels) + u16(16) + bytes(4) + u32(rate << 16)

JOC = bytes.fromhex('1800200f00010c')
BASE = bytes.fromhex('1800200f00')
BTRT = box('btrt', bytes(12))


def ec3(config=JOC, channels=2, rate=48000):
    return box('ec-3', audio_header(channels, rate) + box('dec3', config) + BTRT)

AAC = box('mp4a', audio_header() + full('esds', b'') + BTRT)
VIDEO = track(1, 'vide', VIDEO_ENTRY, 120000, 4000, 30000)


def movie(name, entries):
    audio = b''.join(track(i, 'soun', entry, duration, duration_ms, rate)
                     for i, entry, duration, duration_ms, rate in entries)
    data = box('ftyp', b'isom' + u32(0) + b'isom') + box('moov', MVHD + VIDEO + audio)
    (ROOT / name).write_bytes(data)


def main():
    e = (0xf1234567, ec3(), 216000, 4500, 48000)
    a = (23, AAC, 198000, 4000, 48000)
    movie('eac3_joc.mp4', [e])
    movie('eac3_then_aac.mp4', [e, a])
    movie('aac_then_eac3.mp4', [a, e])
    movie('eac3_plain.mp4', [(2, ec3(BASE, channels=9, rate=44100), 216000, 4500, 48000)])
    movie('eac3_half_rate.mp4', [(2, ec3(bytes.fromhex('1800e00f00'), rate=24000), 108000, 4500, 24000)])
    movie('eac3_unknown_rate.mp4', [(2, ec3(bytes.fromhex('1800e00f00'), rate=0), 108000, 4500, 24000)])
    # One independent 5.1 service plus Lrs/Rrs dependent channels: eight channels.
    movie('eac3_dependent.mp4', [(2, ec3(bytes.fromhex('1800200f0202')), 216000, 4500, 48000)])
    movie('eac3_multi_service.mp4', [(2, ec3(bytes.fromhex('1801200f0060b200')), 216000, 4500, 48000)])
    for name, config in [('header', b'\x18'), ('substream', BASE[:-1]), ('joc', JOC[:-1]), ('dependent', bytes.fromhex('1800200f02'))]:
        movie('eac3_truncated_' + name + '.mp4', [(2, ec3(config), 216000, 4500, 48000), a])
    movie('eac3_missing_dec3.mp4', [(2, box('ec-3', audio_header() + BTRT), 216000, 4500, 48000), a])
    movie('eac3_duplicate_dec3.mp4', [(2, box('ec-3', audio_header() + box('dec3', BASE) * 2), 216000, 4500, 48000), a])
    movie('unsupported_then_aac.mp4', [(2, box('zzzz', audio_header()), 216000, 4500, 48000), a])
    movie('aac_then_unsupported.mp4', [a, (2, box('zzzz', audio_header()), 216000, 4500, 48000)])

if __name__ == '__main__':
    main()
