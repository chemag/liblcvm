#!/usr/bin/env python3
"""Deterministic metadata-only fixtures for track selection and CSV schemas."""
import sys
sys.dont_write_bytecode = True
import create_eac3_fixtures as m


def main():
    e = (0xffffffff, m.ec3(), 216000, 4500, 48000)
    aac441 = m.box('mp4a', m.audio_header(rate=44100) + m.full('esds', b'') + m.BTRT)
    a = (23, aac441, 220500, 4000, 44100)
    m.movie('multi_audio_max_id.mp4', [e, a])
    video = m.VIDEO
    m.VIDEO = m.track(7, 'vide', m.VIDEO_ENTRY, 120000, 4000, 30000)
    m.movie('multi_audio_video_id7.mp4', [e, a])
    m.VIDEO = video
    m.movie('multi_audio_no_audio.mp4', [])
    m.movie('multi_audio_duplicate_id.mp4', [(23, m.ec3(), 216000, 4500, 48000), a])
    original = m.MVHD
    m.MVHD = original[:20] + m.u32(0) + original[24:]
    m.movie('multi_audio_mdhd_fallback.mp4', [e, a])
    m.MVHD = original

if __name__ == '__main__':
    main()
