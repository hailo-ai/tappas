"""
Source: https://gist.github.com/jackersson/7c476f6293cb74ff0d97101304a005c0

Loops gstreamer video

Usage:
    python gst_loop_playback.py -l "filesrc location=video0.mp4 ! decodebin ! videoconvert ! gtksink sync=False" -r 2
"""
import argparse
import logging
import traceback

import gi

gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
from gi.repository import Gst, GObject, GstBase

Gst.init(None)

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

INFINITE_LOOP = -1


class UserData:
    def __init__(self, pipeline: Gst.Pipeline, loop: GObject.MainLoop, prerolled: bool = False,
                 max_loops_num: int = 1):
        self.pipeline = pipeline
        self.prerolled = prerolled
        self.current_loop_id = 1
        self.max_loops_num = max_loops_num

        self.loop = loop

    def quit(self):
        self.loop.quit()


def bus_message_handler(bus: Gst.Bus, message: Gst.Message, user_data: UserData):
    message_type = message.type
    logger.info(message_type)

    if message_type == Gst.MessageType.ASYNC_DONE:
        if not user_data.prerolled:
            user_data.pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.SEGMENT, 0)
            user_data.prerolled = True
    elif message_type == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        logger.error(f"{err}: {debug}")
        user_data.loop.quit()
    elif message_type == Gst.MessageType.WARNING:
        err, debug = message.parse_warning()
        logger.warning(f"{err}: {debug}")
        user_data.quit()
    elif message_type in (Gst.MessageType.SEGMENT_DONE, Gst.MessageType.EOS):
        if user_data.max_loops_num == INFINITE_LOOP or user_data.current_loop_id < user_data.max_loops_num:
            user_data.pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.SEGMENT, 0)
            user_data.current_loop_id += 1
            logger.info(f"Loop... {user_data.current_loop_id}/{user_data.max_loops_num}")
        else:
            logger.info("Video ended")
            user_data.quit()
    else:
        pass

    return True


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--gst_launch", required=True, help="Gst-Launch String")
    parser.add_argument("-r", "--repeat", required=False, default=INFINITE_LOOP,
                        help="Max num of pipeline repetitions (loops) - Default never stop")

    return parser.parse_args()


def main():
    args = parse_args()
    pipeline = Gst.parse_launch(args.gst_launch)

    bus = pipeline.get_bus()
    bus.add_signal_watch()

    loop = GObject.MainLoop()

    bus.connect("message", bus_message_handler, UserData(pipeline, loop, max_loops_num=int(args.repeat)))
    pipeline.set_state(Gst.State.PLAYING)

    try:
        loop.run()
    except:
        traceback.print_exc()

    loop.quit()
    pipeline.set_state(Gst.State.NULL)


if __name__ == '__main__':
    main()
