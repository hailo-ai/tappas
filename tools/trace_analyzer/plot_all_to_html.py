import argparse
import pandas as pd
import plotly.express as px
from pathlib import Path

XTICKS = 40
YTICKS = 20


def plot_queuelevel(queuelevel_log):
    df = pd.read_csv(queuelevel_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 10]]  # only relevant columns
    df.columns = ['s_time', 'queue', 's_size_buffers']  # rename columns
    # clean data
    df['queue'] = df['queue'].apply(lambda s: s.replace("queue=(string)", ""))
    df['s_size_buffers'] = df['s_size_buffers'].apply(lambda s: s.replace(
        "size_buffers=(uint)", "")).apply(lambda s: s.replace(",", ""))
    df['size_buffers'] = pd.to_numeric(
        df['s_size_buffers'])  # convert to numeric format
    df['time'] = pd.to_datetime(df['s_time'])  # convert to datetime format
    fig = px.line(df, x="time", y="size_buffers", color="queue")
    fig.update_layout(title_text="Queue Level")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Queue Level (buffers)", nticks=YTICKS)
    return fig


def plot_thread_cpuusage(thread_cpuusage_log):
    df = pd.read_csv(thread_cpuusage_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]
    # rename columns
    df.columns = ['s_time', 'thread_name', 's_cpu_usage']
    df['thread_name'] = df['thread_name'].apply(
        lambda s: s.replace("name=(string)", ""))
    df['s_cpu_usage'] = df['s_cpu_usage'].apply(lambda s: s.replace(
        "cpu_usage=(double)", "")).apply(lambda s: s.replace(",", ""))
    df['time'] = pd.to_datetime(df['s_time'])
    df['cpu_usage'] = pd.to_numeric(df['s_cpu_usage'])
    fig = px.line(df, x="time", y="cpu_usage", color="thread_name")
    fig.update_layout(title_text="Thread CPU Usage")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="CPU Usage (%)", nticks=YTICKS)
    return fig


def plot_framerate(framerate_log):
    df = pd.read_csv(framerate_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]
    # rename columns
    df.columns = ['s_time', 'pad', 's_framerate']
    df['time'] = pd.to_datetime(df['s_time'])
    df['pad'] = df['pad'].apply(lambda s: s.replace("pad=(string)", ""))
    df['s_framerate'] = df['s_framerate'].apply(lambda s: s.replace(
        "fps=(uint)", "")).apply(lambda s: s.replace(";", ""))
    df['framerate'] = pd.to_numeric(df['s_framerate'])
    fig = px.line(df, x="time", y="framerate", color="pad")
    fig.update_layout(title_text="Framerate")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Framerate (fps)", nticks=YTICKS)
    return fig


def plot_bitrate(bitrate_log):
    df = pd.read_csv(bitrate_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]
    # rename columns
    df.columns = ['s_time', 'pad', 's_bitrate']
    df['time'] = pd.to_datetime(df['s_time'])
    df['pad'] = df['pad'].apply(lambda s: s.replace("pad=(string)", ""))
    df['s_bitrate'] = df['s_bitrate'].apply(lambda s: s.replace(
        "bitrate=(guint64)", "")).apply(lambda s: s.replace(";", ""))
    df['bitrate'] = pd.to_numeric(df['s_bitrate'])
    fig = px.line(df, x="time", y="bitrate", color="pad")
    fig.update_layout(title_text="Bitrate")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Bitrate (bps)", nticks=YTICKS)
    return fig


def plot_cpuusage(cpuusage_log):
    df = pd.read_csv(cpuusage_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]
    # rename columns
    df.columns = ['s_time', 'core', 's_cpu_usage']
    df['core'] = df['core'].apply(
        lambda s: s.replace("number=(uint)", ""))
    df['s_cpu_usage'] = df['s_cpu_usage'].apply(lambda s: s.replace(
        "load=(double)", "")).apply(lambda s: s.replace(";", ""))
    df['time'] = pd.to_datetime(df['s_time'])
    df['cpu_usage'] = pd.to_numeric(df['s_cpu_usage'])
    fig = px.line(df, x="time", y="cpu_usage", color="core")
    fig.update_layout(title_text="CPU Usage")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="CPU Usage (%)", nticks=YTICKS)
    return fig


def plot_interlatency(interlatency_log):
    df = pd.read_csv(interlatency_log, sep=' ', header=None)
    df = df.iloc[:, [0, 8, 9]]
    # rename columns
    df.columns = ['s_time', 'destination_pad', 's_proctime']
    df['destination_pad'] = df['destination_pad'].apply(
        lambda s: s.replace("to_pad=(string)", ""))
    df['s_proctime'] = df['s_proctime'].apply(lambda s: s.replace(
        "time=(string)", "")).apply(lambda s: s.replace(";", ""))
    df['proctime'] = pd.to_datetime(df['s_proctime'])
    df['time'] = pd.to_datetime(df['s_time'])
    fig = px.line(df, x="time", y="proctime", color="destination_pad")
    fig.update_layout(title_text="Interlatency")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Time (s)", nticks=YTICKS)
    return fig


def plot_proctime(proctime_log):
    df = pd.read_csv(proctime_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]
    # rename columns
    df.columns = ['s_time', 'element', 's_proctime']
    df['element'] = df['element'].apply(
        lambda s: s.replace("element=(string)", ""))
    df['s_proctime'] = df['s_proctime'].apply(lambda s: s.replace(
        "time=(string)", "")).apply(lambda s: s.replace(";", ""))
    df['proctime'] = pd.to_datetime(df['s_proctime'])
    df['time'] = pd.to_datetime(df['s_time'])
    fig = px.line(df, x="time", y="proctime", color="element")
    fig.update_xaxes(nticks=XTICKS)
    fig.update_layout(title_text="Processing Time")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Processing Time (s)", nticks=YTICKS)
    return fig


def plot_scheduletime(scheduletime_log):
    df = pd.read_csv(scheduletime_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]
    # rename columns
    df.columns = ['s_time', 'pad', 's_scheduletime']
    df['pad'] = df['pad'].apply(
        lambda s: s.replace("pad=(string)", ""))
    df['s_scheduletime'] = df['s_scheduletime'].apply(lambda s: s.replace(
        "time=(string)", "")).apply(lambda s: s.replace(";", ""))
    df['scheduletime'] = pd.to_datetime(df['s_scheduletime'])
    df['time'] = pd.to_datetime(df['s_time'])
    fig = px.line(df, x="time", y="scheduletime", color="pad")
    fig.update_xaxes(nticks=XTICKS)
    fig.update_layout(title_text="Schedule Time")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Schedule Time(s)", nticks=YTICKS)
    return fig


def main():
    tracer_to_plotter = {
        'queuelevel.log': plot_queuelevel,
        'threadmonitor.log': plot_thread_cpuusage,
        'framerate.log': plot_framerate,
        'bitrate.log': plot_bitrate,
        'cpuusage.log': plot_cpuusage,
        'interlatency.log': plot_interlatency,
        'proctime.log': plot_proctime,
        'scheduletime.log': plot_scheduletime,
    }

    parser = argparse.ArgumentParser(description='Plot traces')
    parser.add_argument('-p', '--path', type=str, help='Path to traces dir')
    args = parser.parse_args()
    traces_dir = args.path

    trace_log_files = list(Path(traces_dir).rglob('*.log'))
    with open(f"{traces_dir}/graphs.html", 'a') as f:
        for trace_log_file in trace_log_files:
            if trace_log_file.name in tracer_to_plotter.keys():
                f.write(tracer_to_plotter[trace_log_file.name](
                    f"{traces_dir}/{trace_log_file.name}").to_html(full_html=False, include_plotlyjs='cdn'))

    print(f"{traces_dir}/graphs.html created")


if __name__ == "__main__":
    main()
