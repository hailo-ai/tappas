import ffmpeg
import argparse

def parse_downloader_args():
    parser = argparse.ArgumentParser(description='Get bitrate')
    parser.add_argument('-v', '--video-path', required=True, help='Path to the video')
     
    return parser.parse_args()

def get_bitrate(file):
    probe = ffmpeg.probe(file)
    video_stream = next(s for s in probe['streams'] if s['codec_type'] == 'video')
    bitrate_in_kbs = int(video_stream['bit_rate']) // 1000

    return bitrate_in_kbs

if __name__ == "__main__":
    args = parse_downloader_args()
    bitrate = get_bitrate(args.video_path)
    
    print(f"{bitrate}")
