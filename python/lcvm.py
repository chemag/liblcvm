#!/usr/bin/env python3

"""lcvm.py

Video analysis module utilizing ISOBMFF information.
Accelerated by the liblcvm C++ shared library (liblcvm.so).
"""

import argparse
import csv
import importlib
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "build"))
import liblcvm


default_values = {
    "nruns": 1,
    "debug": 0,
    "outfile_timestamps_sort_pts": True,
    "outfile_timestamps": "outfile_timestamps.csv",
    "infile_list": [],
    "outfile": None,
    "policy": None,
}


csv_header = [
    "infile",
    "filesize",
    "bitrate_bps",
    "width",
    "height",
    "video_codec_type",
    "horizresolution",
    "vertresolution",
    "depth",
    "chroma_format",
    "bit_depth_luma",
    "bit_depth_chroma",
    "video_full_range_flag",
    "colour_primaries",
    "transfer_characteristics",
    "matrix_coeffs",
    "num_video_frames",
    "frame_rate_fps_median",
    "frame_rate_fps_average",
    "frame_rate_fps_reverse_average",
    "frame_rate_fps_stddev",
    "video_freeze",
    "audio_video_ratio",
    "duration_video_sec",
    "duration_audio_sec",
    "timescale_video_hz",
    "timescale_audio_hz",
    "pts_duration_sec_average",
    "pts_duration_sec_median",
    "pts_duration_sec_stddev",
    "pts_duration_sec_mad",
    "frame_drop_count",
    "frame_drop_ratio",
    "normalized_frame_drop_average_length",
    "frame_drop_length_percentile_50",
    "frame_drop_length_percentile_90",
    "frame_drop_length_consecutive_2",
    "frame_drop_length_consecutive_5",
    "num_video_keyframes",
    "key_frame_ratio",
    "audio_type",
    "channel_count",
    "sample_rate",
    "sample_size",
    "policy_version",
    "warn_list",
    "error_list",
]


def get_options(argv):
    """Generic option parser.

    Args:
        argv: list containing arguments

    Returns:
        Namespace - An argparse.ArgumentParser-generated option object
    """
    # init parser
    # usage = 'usage: %prog [options] arg1 arg2'
    # parser = argparse.OptionParser(usage=usage)
    # parser.print_help() to get argparse.usage (large help)
    # parser.print_usage() to get argparse.usage (just usage line)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-d",
        "--debug",
        type=int,  # Change to accept a value
        dest="debug",
        default=default_values["debug"],
        help="Set debug level (e.g., -d 1)",
    )
    parser.add_argument(
        dest="infile_list",
        type=str,
        nargs="+",
        default=default_values["infile_list"],
        metavar="input-file",
        help="input file",
    )
    parser.add_argument(
        "-o",
        "--outfile",
        dest="outfile",
        type=str,
        default=default_values["outfile"],
        metavar="output-file",
        help="output file",
    )
    parser.add_argument(
        "-p",
        "--policy",
        dest="policy",
        type=str,
        default=default_values["policy"],
        metavar="policy-file",
        help="policy file for validation rules",
    )

    # do the parsing
    options = parser.parse_args(argv[1:])
    return options


def run_policy(policy_str, keys, vals, debug):
    """Run policy validation on the extracted metrics.

    Args:
        policy_str: Policy string to evaluate
        keys: List of metric names
        vals: List of metric values
        debug: Debug level

    Returns:
        Tuple of (result_code, warnings_list, errors_list, version)
    """
    try:
        # Check if policy_runner is available (requires ADD_POLICY=ON)
        if not hasattr(liblcvm, "policy_runner"):
            if debug > 0:
                print(
                    "Warning: policy_runner not available. Library was built without ADD_POLICY=ON."
                )
            return None

        result, warnings, errors, version = liblcvm.policy_runner(
            policy_str, keys, vals
        )

        if debug > 0:
            print(f"Policy validation result: {result}")
            if version:
                print(f"Policy version: {version}")
            if warnings:
                print(f"Warnings: {len(warnings)}")
                for warn in warnings:
                    print(f"  - {warn}")
            if errors:
                print(f"Errors: {len(errors)}")
                for err in errors:
                    print(f"  - {err}")

        return (result, warnings, errors, version)

    except Exception as e:
        print(f"Error running policy validation: {e}")
        return None


def dump_output(infile, outfp, file_info, debug, policy_str=None):

    # Access the frame information
    frame_info = file_info.get_frame()
    filesize = frame_info.get_filesize()
    bitrate_bps = frame_info.get_bitrate_bps()
    width = frame_info.get_width()
    height = frame_info.get_height()
    video_codec_type = frame_info.get_video_codec_type()
    horizresolution = frame_info.get_horizresolution()
    vertresolution = frame_info.get_vertresolution()
    depth = frame_info.get_depth()
    chroma_format = frame_info.get_chroma_format()
    bit_depth_luma = frame_info.get_bit_depth_luma()
    bit_depth_chroma = frame_info.get_bit_depth_chroma()
    video_full_range_flag = frame_info.get_video_full_range_flag()
    colour_primaries = frame_info.get_colour_primaries()
    transfer_characteristics = frame_info.get_transfer_characteristics()
    matrix_coeffs = frame_info.get_matrix_coeffs()

    # Access the timing information
    timing_info = file_info.get_timing()
    video_freeze = timing_info.get_video_freeze()
    audio_video_ratio = timing_info.get_audio_video_ratio()
    duration_video_sec = timing_info.get_duration_video_sec()
    duration_audio_sec = timing_info.get_duration_audio_sec()
    timescale_video_hz = timing_info.get_timescale_video_hz()
    timescale_audio_hz = timing_info.get_timescale_audio_hz()
    pts_duration_sec_average = timing_info.get_pts_duration_sec_average()
    pts_duration_sec_median = timing_info.get_pts_duration_sec_median()
    pts_duration_sec_stddev = timing_info.get_pts_duration_sec_stddev()
    pts_duration_sec_mad = timing_info.get_pts_duration_sec_mad()
    num_video_frames = timing_info.get_num_video_frames()
    frame_rate_fps_median = timing_info.get_frame_rate_fps_median()
    frame_rate_fps_average = timing_info.get_frame_rate_fps_average()
    frame_rate_fps_reverse_average = timing_info.get_frame_rate_fps_reverse_average()
    frame_rate_fps_stddev = timing_info.get_frame_rate_fps_stddev()
    frame_drop_count = timing_info.get_frame_drop_count()
    frame_drop_ratio = timing_info.get_frame_drop_ratio()
    normalized_frame_drop_average_length = (
        timing_info.get_normalized_frame_drop_average_length()
    )

    # Calculate percentile list
    percentile_list = [50, 90]
    frame_drop_length_percentile_list = []
    frame_drop_length_percentile_list = timing_info.calculate_percentile_list(
        percentile_list, debug
    )

    # Calculate consecutive list
    consecutive_list = [2, 5]
    frame_drop_length_consecutive = []
    frame_drop_length_consecutive = timing_info.calculate_consecutive_list(
        consecutive_list, debug
    )

    # get video structure info
    num_video_keyframes = timing_info.get_num_video_keyframes()
    key_frame_ratio = timing_info.get_key_frame_ratio()

    # Access the audio information
    audio_info = file_info.get_audio()
    audio_type = audio_info.get_audio_type()
    channel_count = audio_info.get_channel_count()
    sample_rate = audio_info.get_sample_rate()
    sample_size = audio_info.get_sample_size()

    # Run policy validation if policy string is provided
    policy_version = ""
    warn_list_str = ""
    error_list_str = ""

    if policy_str:
        keys = [
            "filesize",
            "bitrate_bps",
            "width",
            "height",
            "video_codec_type",
            "horizresolution",
            "vertresolution",
            "depth",
            "chroma_format",
            "bit_depth_luma",
            "bit_depth_chroma",
            "video_full_range_flag",
            "colour_primaries",
            "transfer_characteristics",
            "matrix_coeffs",
            "num_video_frames",
            "frame_rate_fps_median",
            "frame_rate_fps_average",
            "frame_rate_fps_reverse_average",
            "frame_rate_fps_stddev",
            "video_freeze",
            "audio_video_ratio",
            "duration_video_sec",
            "duration_audio_sec",
            "timescale_video_hz",
            "timescale_audio_hz",
            "pts_duration_sec_average",
            "pts_duration_sec_median",
            "pts_duration_sec_stddev",
            "pts_duration_sec_mad",
            "frame_drop_count",
            "frame_drop_ratio",
            "normalized_frame_drop_average_length",
            "frame_drop_length_percentile_50",
            "frame_drop_length_percentile_90",
            "frame_drop_length_consecutive_2",
            "frame_drop_length_consecutive_5",
            "num_video_keyframes",
            "key_frame_ratio",
            "audio_type",
            "channel_count",
            "sample_rate",
            "sample_size",
        ]
        vals = [
            filesize,
            bitrate_bps,
            width,
            height,
            video_codec_type,
            horizresolution,
            vertresolution,
            depth,
            chroma_format,
            bit_depth_luma,
            bit_depth_chroma,
            video_full_range_flag,
            colour_primaries,
            transfer_characteristics,
            matrix_coeffs,
            num_video_frames,
            frame_rate_fps_median,
            frame_rate_fps_average,
            frame_rate_fps_reverse_average,
            frame_rate_fps_stddev,
            int(video_freeze),
            audio_video_ratio,
            duration_video_sec,
            duration_audio_sec,
            timescale_video_hz,
            timescale_audio_hz,
            pts_duration_sec_average,
            pts_duration_sec_median,
            pts_duration_sec_stddev,
            pts_duration_sec_mad,
            frame_drop_count,
            frame_drop_ratio,
            normalized_frame_drop_average_length,
            frame_drop_length_percentile_list[0],
            frame_drop_length_percentile_list[1],
            frame_drop_length_consecutive[0],
            frame_drop_length_consecutive[1],
            num_video_keyframes,
            key_frame_ratio,
            audio_type,
            channel_count,
            sample_rate,
            sample_size,
        ]

        policy_result = run_policy(policy_str, keys, vals, debug)
        if policy_result:
            result_code, warnings, errors, version = policy_result

            policy_version = version if version else ""
            # Join warnings and errors with semicolon for CSV
            warn_list_str = "; ".join(warnings) if warnings else ""
            error_list_str = "; ".join(errors) if errors else ""

    # Dump all output
    # Write to the output file
    outfp.write(f"{infile}")
    outfp.write(f",{filesize}")
    outfp.write(f",{bitrate_bps:.6f}")  # Format the double with 6 decimal places
    outfp.write(f",{width:.6f}")
    outfp.write(f",{height:.6f}")
    outfp.write(f",{video_codec_type}")
    outfp.write(f",{horizresolution}")
    outfp.write(f",{vertresolution}")
    outfp.write(f",{depth}")
    outfp.write(f",{chroma_format}")
    outfp.write(f",{bit_depth_luma}")
    outfp.write(f",{bit_depth_chroma}")
    outfp.write(f",{video_full_range_flag}")
    outfp.write(f",{colour_primaries}")
    outfp.write(f",{transfer_characteristics}")
    outfp.write(f",{matrix_coeffs}")

    outfp.write(f",{num_video_frames}")
    outfp.write(f",{frame_rate_fps_median:.6f}")
    outfp.write(f",{frame_rate_fps_average:.6f}")
    outfp.write(f",{frame_rate_fps_reverse_average:.6f}")
    outfp.write(f",{frame_rate_fps_stddev:.6f}")
    outfp.write(f",{1 if video_freeze else 0}")
    outfp.write(f",{audio_video_ratio:.6f}")
    outfp.write(f",{duration_video_sec:.6f}")
    outfp.write(f",{duration_audio_sec:.6f}")
    outfp.write(f",{timescale_video_hz}")
    outfp.write(f",{timescale_audio_hz}")
    outfp.write(f",{pts_duration_sec_average:.6f}")
    outfp.write(f",{pts_duration_sec_median:.6f}")
    outfp.write(f",{pts_duration_sec_stddev:.6f}")
    outfp.write(f",{pts_duration_sec_mad:.6f}")
    outfp.write(f",{frame_drop_count}")
    outfp.write(f",{frame_drop_ratio:.6f}")
    outfp.write(f",{normalized_frame_drop_average_length:.6f}")
    outfp.write(f",{frame_drop_length_percentile_list[0]:.6f}")
    outfp.write(f",{frame_drop_length_percentile_list[1]:.6f}")
    outfp.write(f",{frame_drop_length_consecutive[0]}")
    outfp.write(f",{frame_drop_length_consecutive[1]}")
    outfp.write(f",{num_video_keyframes}")
    outfp.write(f",{key_frame_ratio:.6f}")
    outfp.write(f",{audio_type}")
    outfp.write(f",{channel_count}")
    outfp.write(f",{sample_rate}")
    outfp.write(f",{sample_size}")
    outfp.write(f",{policy_version}")
    outfp.write(f',"{warn_list_str}"')
    outfp.write(f',"{error_list_str}"')
    outfp.write("\n")


def capture_timestamps(
    infile,
    file_info,
    frame_num_orig_list_dict,
    stts_unit_list_dict,
    ctts_unit_list_dict,
    dts_sec_list_dict,
    pts_sec_list_dict,
    pts_duration_sec_list_dict,
    pts_duration_delta_sec_list_dict,
    pts_framerate_list_dict,
):
    # Access the timing information
    timing_info = file_info.get_timing()

    frame_num_orig_list = timing_info.get_frame_num_orig_list()
    stts_unit_list = timing_info.get_stts_unit_list()
    ctts_unit_list = timing_info.get_ctts_unit_list()
    dts_sec_list = timing_info.get_dts_sec_list()
    pts_sec_list = timing_info.get_pts_sec_list()
    pts_duration_sec_list = timing_info.get_pts_duration_sec_list()
    pts_duration_delta_sec_list = timing_info.get_pts_duration_sec_list()
    pts_framerate_list = timing_info.get_pts_framerate_list()

    # Store the values in dictionaries
    frame_num_orig_list_dict[infile] = frame_num_orig_list
    stts_unit_list_dict[infile] = stts_unit_list
    ctts_unit_list_dict[infile] = ctts_unit_list
    dts_sec_list_dict[infile] = dts_sec_list
    pts_sec_list_dict[infile] = pts_sec_list
    pts_duration_sec_list_dict[infile] = pts_duration_sec_list
    pts_duration_delta_sec_list_dict[infile] = pts_duration_delta_sec_list
    pts_framerate_list_dict[infile] = pts_framerate_list


def write_timestamps_to_file(
    outfile_timestamps,
    frame_num_orig_list_dict,
    stts_unit_list_dict,
    ctts_unit_list_dict,
    dts_sec_list_dict,
    pts_sec_list_dict,
    pts_duration_sec_list_dict,
    pts_duration_delta_sec_list_dict,
    pts_framerate_list_dict,
):
    # Write the inter-frame timestamps to the output file.

    # Get the number of frames of the longest file
    max_number_of_frames = 0
    for frame_num_orig_list in frame_num_orig_list_dict.values():
        max_number_of_frames = max(max_number_of_frames, len(frame_num_orig_list))

    # Open outfile_timestamps
    try:
        with open(outfile_timestamps, "w") as outtsfp:
            # Dump the file names (header row)
            outtsfp.write("frame_num,frame_num_orig")
            for infile in stts_unit_list_dict.keys():
                outtsfp.write(
                    f",stts_{infile},ctts_{infile},dts_{infile},pts_{infile},pts_duration_{infile}"
                )
            outtsfp.write("\n")

            # Dump the columns of inter-frame timestamps
            for frame_num in range(max_number_of_frames):
                outtsfp.write(f"{frame_num}")

                # Dump frame_num_orig_list[frame_num]
                for frame_num_orig_list in frame_num_orig_list_dict.values():
                    if frame_num < len(frame_num_orig_list):
                        outtsfp.write(f",{frame_num_orig_list[frame_num]}")
                    else:
                        outtsfp.write(",")

                # Dump stts_unit_list[frame_num]
                for stts_unit_list in stts_unit_list_dict.values():
                    if frame_num < len(stts_unit_list):
                        outtsfp.write(f",{stts_unit_list[frame_num]}")
                    else:
                        outtsfp.write(",")

                # Dump ctts_unit_list[frame_num]
                for ctts_unit_list in ctts_unit_list_dict.values():
                    if frame_num < len(ctts_unit_list):
                        outtsfp.write(f",{ctts_unit_list[frame_num]}")
                    else:
                        outtsfp.write(",")

                # Dump dts_sec_list[frame_num]
                for dts_sec_list in dts_sec_list_dict.values():
                    if frame_num < len(dts_sec_list):
                        outtsfp.write(f",{dts_sec_list[frame_num]:.6f}")
                    else:
                        outtsfp.write(",")

                # Dump pts_sec_list[frame_num]
                for pts_sec_list in pts_sec_list_dict.values():
                    if frame_num < len(pts_sec_list):
                        outtsfp.write(f",{pts_sec_list[frame_num]:.6f}")
                    else:
                        outtsfp.write(",")

                # Dump pts_duration_sec_list[frame_num]
                for pts_duration_sec_list in pts_duration_sec_list_dict.values():
                    if frame_num < len(pts_duration_sec_list):
                        outtsfp.write(f",{pts_duration_sec_list[frame_num]:.6f}")
                    else:
                        outtsfp.write(",")

                # Dump pts_duration_delta_sec_list[frame_num]
                for (
                    pts_duration_delta_sec_list
                ) in pts_duration_delta_sec_list_dict.values():
                    if frame_num < len(pts_duration_delta_sec_list):
                        outtsfp.write(f",{pts_duration_delta_sec_list[frame_num]:.6f}")
                    else:
                        outtsfp.write(",")

                # Dump pts_framerate_list[frame_num]
                for pts_framerate_list in pts_framerate_list_dict.values():
                    if frame_num < len(pts_framerate_list):
                        outtsfp.write(f",{pts_framerate_list[frame_num]:.6f}")
                    else:
                        outtsfp.write(",")

                outtsfp.write("\n")

    except IOError:
        print(f'Could not open output file: "{outfile_timestamps}"')
        return -1


def parse_files(
    infile_list,
    outfile,
    outfile_timestamps,
    outfile_timestamps_sort_pts,
    debug,
    policy_file=None,
):
    # Open outfile
    if outfile is None or (len(outfile) == 1 and outfile == "-"):
        # Use stdout
        outfp = sys.stdout
    else:
        try:
            outfp = open(outfile, "w")
        except IOError:
            # Did not work
            sys.stderr.write(f'Could not open output file: "{outfile}"\n')
            return -1

    # Read policy file if provided
    policy_str = None
    if policy_file:
        try:
            with open(policy_file, "r") as pf:
                policy_str = pf.read()
                if debug > 0:
                    print(f"Loaded policy from: {policy_file}")
        except IOError as e:
            sys.stderr.write(f'Could not read policy file: "{policy_file}": {e}\n')
            return -1

    # 1. Write CSV header
    csv_writer = csv.writer(outfp)
    csv_writer.writerow(csv_header)

    # 2. Initialize dictionaries
    frame_num_orig_list_dict = {}
    stts_unit_list_dict = {}
    ctts_unit_list_dict = {}
    dts_sec_list_dict = {}
    pts_sec_list_dict = {}
    pts_duration_sec_list_dict = {}
    pts_duration_delta_sec_list_dict = {}
    pts_framerate_list_dict = {}

    # 2.1. analyze file
    config = liblcvm.LiblcvmConfig()

    # 2.1.1. Iterate over each file in the infile_list
    for infile in infile_list:
        print(f"Input file: {infile}")

        # Call the parse method from the IsobmffFileInformation class
        try:
            file_info = liblcvm.parse(infile, config)
            print("File information parsed successfully.")

            dump_output(infile, outfp, file_info, debug, policy_str)
            # 2.4. capture outfile timestamps
            if outfile_timestamps is not None:
                capture_timestamps(
                    infile,
                    file_info,
                    frame_num_orig_list_dict,
                    stts_unit_list_dict,
                    ctts_unit_list_dict,
                    dts_sec_list_dict,
                    pts_sec_list_dict,
                    pts_duration_sec_list_dict,
                    pts_duration_delta_sec_list_dict,
                    pts_framerate_list_dict,
                )

        except Exception as e:
            print(f"An error occurred with file {infile}: {e}")

    # 3. dump outfile timestamps
    if outfile_timestamps is not None:
        write_timestamps_to_file(
            outfile_timestamps,
            frame_num_orig_list_dict,
            stts_unit_list_dict,
            ctts_unit_list_dict,
            dts_sec_list_dict,
            pts_sec_list_dict,
            pts_duration_sec_list_dict,
            pts_duration_delta_sec_list_dict,
            pts_framerate_list_dict,
        )

    if outfp is not sys.stdout:
        outfp.close()


def main(argv):
    # parse options
    options = get_options(argv)
    # get outfile
    if options.outfile == "-" or options.outfile is None:
        options.outfile = "/dev/fd/1"
    # print results
    if options.debug > 0:
        print(options)

    options.nruns = default_values["nruns"]
    options.outfile_timestamps = default_values["outfile_timestamps"]
    options.outfile_timestamps_sort_pts = default_values["outfile_timestamps_sort_pts"]

    for i in range(options.nruns):
        parse_files(
            options.infile_list,
            options.outfile,
            options.outfile_timestamps,
            options.outfile_timestamps_sort_pts,
            options.debug,
            options.policy,
        )


if __name__ == "__main__":
    main(sys.argv)
