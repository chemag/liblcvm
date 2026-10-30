#!/usr/bin/env python3
"""Generate deterministic metadata-only multi-video regression inputs."""
import sys
sys.dont_write_bytecode = True
from pathlib import Path
import struct
import create_eac3_fixtures as m

ROOT = Path(__file__).resolve().parent


def video(track_id, width, height, frames, rate, seconds, keys=(1,), ctts=None,
          entry_type='avc1', config=True):
    header = (bytes(6) + m.u16(1) + bytes(16) + m.u16(width) + m.u16(height) +
              m.u32(0x480000) * 2 + m.u32(0) + m.u16(1) + bytes(32) +
              m.u16(24) + m.u16(0xffff))
    entry = m.box(entry_type, header + (m.box('avcC', bytes.fromhex('0142001effe000')) if config else b''))
    stsd = m.full('stsd', m.u32(1) + entry)
    stts = m.full('stts', m.u32(1) + m.u32(frames) + m.u32(1000))
    stss = m.full('stss', m.u32(len(keys)) + b''.join(m.u32(k) for k in keys))
    offsets = b''
    if ctts is not None:
        offsets = m.full('ctts', m.u32(len(ctts)) + b''.join(
            m.u32(1) + struct.pack('>i', n) for n in ctts), 1 << 24)
    tkhd = m.full('tkhd', m.u32(0) * 2 + m.u32(track_id) + m.u32(0) +
                  m.u32(int(seconds * 1000)) + bytes(16) + m.MATRIX +
                  m.u32(width << 16) + m.u32(height << 16), 3)
    timescale = rate * 1000
    mdhd = m.full('mdhd', m.u32(0) * 2 + m.u32(timescale) + m.u32(frames * 1000) + bytes(4))
    hdlr = m.full('hdlr', m.u32(0) + b'vide' + bytes(12) + b'\0')
    return m.box('trak', tkhd + m.box('mdia', mdhd + hdlr + m.box('minf',
                 m.box('stbl', stsd + stts + stss + offsets))))


def audio(track_id, eac3=False):
    rate = 48000 if eac3 else 44100
    entry = m.ec3() if eac3 else m.box('mp4a', m.audio_header(rate=rate) + m.full('esds', b'') + m.BTRT)
    return m.track(track_id, 'soun', entry,
                   216000 if eac3 else 141120, 4500 if eac3 else 3000, rate)


def write(name, tracks):
    data = m.box('ftyp', b'isom' + m.u32(0) + b'isom') + m.box('moov', m.MVHD + b''.join(tracks))
    (ROOT / name).write_bytes(data)


def main():
    v1 = video(11, 320, 240, 120, 30, 4, (1, 61))
    v2 = video(21, 640, 360, 72, 24, 3)
    a1, a2 = audio(31, True), audio(41)
    for nv, na in ((1,1), (1,2), (2,1), (2,2), (2,0), (0,1), (1,0), (0,0)):
        write(f'multivideo_{nv}v_{na}a.mp4', [v1,v2][:nv] + [a1,a2][:na])
    write('multivideo_reversed.mp4', [v2,a1,v1,a2])
    high = video(0xffffffff, 320, 240, 120, 30, 4, (1,61))
    write('multivideo_max_id.mp4', [high,v2,a1,a2])
    write('multivideo_duplicate.mp4', [v1,video(11,640,360,72,24,3),a1,a2])
    write('multivideo_unsupported.mp4', [video(11,320,240,120,30,4,entry_type='zzzz'),v2,a1])
    write('multivideo_missing_config.mp4', [v1,video(21,640,360,72,24,3,config=False),a1])
    write('multivideo_ctts.mp4', [video(11,320,240,4,1,4,(1,3),[0,2000,-1000,0]),
                                video(21,640,360,3,1,3,(2,),[2000,0,-2000]),a1])

if __name__ == '__main__':
    main()
