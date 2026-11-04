#!/usr/bin/env python3
"""Tiny metadata fixtures for unsupported-track policy; not FLAC decoding support."""
import sys
sys.dont_write_bytecode = True
import create_eac3_fixtures as m
import create_multivideo_fixtures as v


def audio_description(track_id, stsd):
    tkhd = m.full('tkhd', m.u32(0) * 2 + m.u32(track_id) + m.u32(0) +
                  m.u32(4000) + bytes(16) + m.MATRIX + m.u32(0) * 2, 3)
    mdhd = m.full('mdhd', m.u32(0) * 2 + m.u32(48000) + m.u32(192000) + bytes(4))
    hdlr = m.full('hdlr', m.u32(0) + b'soun' + bytes(12) + b'\0')
    return m.box('trak', tkhd + m.box('mdia', mdhd + hdlr +
                 m.box('minf', m.box('stbl', stsd))))


def main():
    video = v.video(11, 320, 240, 120, 30, 4, (1, 61))
    aac = m.track(31, 'soun', m.AAC, 192000, 4000, 48000)
    dops = b'\0\x05' + m.u16(312) + m.u32(48000) + m.u16(0) + bytes([255, 5, 0, 0, 1, 2, 3, 4])
    opus_entry = m.box('Opus', m.audio_header(5) + m.box('dOps', dops))
    opus = m.track(41, 'soun', opus_entry, 240000, 5000, 48000)
    unknown = m.track(99, 'soun', m.box('fLaC', m.audio_header()), 999000, 999000, 1000)
    v.write('supported_aac_opus.mp4', [video, aac, opus])
    for position, tracks in (
        ('first', [unknown, video, aac, opus]),
        ('middle', [video, aac, unknown, opus]),
        ('last', [video, aac, opus, unknown]),
    ):
        v.write('unknown_audio_' + position + '.mp4', tracks)
    unsupported_video = v.video(77, 640, 360, 10000, 24, 500, entry_type='zzzz')
    for position, tracks in (
        ('first', [unsupported_video, video, aac, opus]),
        ('last', [video, aac, opus, unsupported_video]),
    ):
        v.write('unknown_video_' + position + '.mp4', tracks)
    v.write('unknown_both.mp4', [unsupported_video, unknown, video, aac, opus])
    v.write('only_unknown_audio.mp4', [video, unknown])
    v.write('only_unknown_video.mp4', [unsupported_video, aac, opus])
    malformed_opus = m.track(41, 'soun', m.box('Opus', m.audio_header(5) + m.box('dOps', dops[:-1])), 240000, 5000, 48000)
    v.write('malformed_opus_with_unknown.mp4', [video, aac, unknown, malformed_opus])
    v.write('missing_avcc_with_unknown.mp4', [v.video(11, 320, 240, 120, 30, 4, config=False), unknown, aac])
    v.write('missing_audio_stsd.mp4', [video, aac, audio_description(99, b'')])
    v.write('empty_audio_stsd.mp4', [video, aac, audio_description(99, m.full('stsd', m.u32(0)))])
    flac_entry = m.box('fLaC', m.audio_header() + m.box('dfLa', bytes(38)) + m.BTRT)
    v.write('generic_audio_children.mp4', [video, aac, audio_description(99, m.full('stsd', m.u32(1) + flac_entry))])
    v.write('unknown_then_known_stsd.mp4', [video, audio_description(31, m.full('stsd', m.u32(2) + flac_entry + m.AAC))])
    other = m.box('trak', m.box('mdia', m.full('hdlr', m.u32(0) + b'hint' + bytes(12) + b'\0')))
    v.write('non_av_handler_no_mdhd.mp4', [other, video, aac, opus])
    skipped_audio = m.box('trak', m.box('mdia', m.full('hdlr', m.u32(0) + b'soun' + bytes(12) + b'\0')))
    v.write('skipped_audio_no_metadata.mp4', [skipped_audio, video])


if __name__ == '__main__':
    main()
