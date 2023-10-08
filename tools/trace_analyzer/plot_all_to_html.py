import argparse
import pandas as pd
import plotly.express as px
import subprocess
import re
from pathlib import Path

XTICKS = 40
YTICKS = 20

DOT_TO_SVG_COMMAND_TEMPLATE = 'dot {dot_path} -Tsvg > {output_name}'
DOT_TEMPLATE = '''
<script src="https://cdn.jsdelivr.net/npm/svg-pan-zoom@3.6.1/dist/svg-pan-zoom.min.js"></script>
<div id="container" style="height: 300px; margin: 2.5%; background-color: #f9f9f9; border: 1px solid #ccc; padding: 10px">
<svg id="demo-svg" xmlns="http://www.w3.org/2000/svg" style="display: inline; width: 100%; min-width: inherit; max-width: inherit; height: 100%; min-height: inherit; max-height: inherit; " viewBox="0 0 900 900" version="1.1">
{svg_content}
</svg>

<script>
    // Don't use window.onLoad like this in production, because it can only listen to one function.
    window.onload = function() {{
    // Expose to window namespase for testing purposes
    window.zoomTiger = svgPanZoom('#demo-svg', {{
        zoomEnabled: true,
        controlIconsEnabled: true,
        fit: true,
        center: true,
        // viewportSelector: document.getElementById('demo-svg').querySelector('#g4') // this option will make library to misbehave. Viewport should have no transform attribute
    }});

        }};
</script>
</div>
'''

GST_LAUNCH_TEMPLATE = '''
<style>
    .code-container {{
        border: 1px solid #ccc;
        padding: 10px;
        background-color: #f9f9f9;
        margin: 2.5%;
        position: relative;
        max-height: 150px;
        overflow: auto;
    }}
    .copy-button {{
        position: absolute;
        top: 5px;
        right: 5px;
        background-color: #ddd;
        border: none;
        color: #333;
        padding: 5px 10px;
        cursor: pointer;
    }}
    .copy-button.clicked {{
        background-color: #007bff;
        color: #fff;
    }}
    .code-section {{
        white-space: pre-wrap;
        word-wrap: break-word;
    }}
</style>

<div class="code-container">
    <pre>
<code class="code-section">gst-launch-1.0 {launch_pipeline}</code>
    </pre>
    <button class="copy-button">Copy</button>
</div>

<script>
    document.addEventListener('DOMContentLoaded', function() {{
        const copyButtons = document.querySelectorAll('.copy-button');
        copyButtons.forEach(button => {{
            button.addEventListener('click', function() {{
                const codeBlock = this.parentElement.querySelector('code');
                const tempInput = document.createElement('textarea');
                tempInput.value = codeBlock.textContent;
                document.body.appendChild(tempInput);
                tempInput.select();
                document.execCommand('copy');
                document.body.removeChild(tempInput);

                button.classList.add('clicked');
                setTimeout(function() {{
                    button.classList.remove('clicked');
                }}, 250);
            }});
        }});
    }});
</script>
'''


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

def plot_bufferdrop(bufferdrop_log):
    df = pd.read_csv(bufferdrop_log, sep=' ', header=None)
    df = df.iloc[:, [0, 7, 8]]  # only relevant columns
    df.columns = ['s_time', 'element', 's_buffer_drop']  # rename columns
    # clean data
    df['element'] = df['element'].apply(lambda s: s.replace("name=(string)", ""))
    df['s_buffer_drop'] = df['s_buffer_drop'].apply(lambda s: s.replace(
        "buffers_drop=(uint)", "")).apply(lambda s: s.replace(";", ""))
    df['buffer_drop'] = pd.to_numeric(
        df['s_buffer_drop'])  # convert to numeric format
    df['time'] = pd.to_datetime(df['s_time'])  # convert to datetime format
    fig = px.line(df, x="time", y="buffer_drop", color="element")
    fig.update_layout(title_text="Buffer Drop")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Buffer Drop (buffers)", nticks=YTICKS)
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
    df['timedelta'] = pd.to_timedelta(df['s_proctime'])
    df['interlatency'] = df['timedelta'].dt.total_seconds() * 1000
    df['time'] = pd.to_datetime(df['s_time'])
    fig = px.line(df, x="time", y="interlatency", color="destination_pad")
    fig.update_layout(title_text="Interlatency")
    fig.update_xaxes(title_text="Time", nticks=XTICKS)
    fig.update_yaxes(title_text="Time (ms)", nticks=YTICKS)

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
    df['timedelta'] = pd.to_timedelta(df['s_proctime'])
    df['proctime'] = df['timedelta'].dt.total_seconds() * 1000
    df['time'] = pd.to_datetime(df['s_time'])
    fig = px.line(df, x="time", y="proctime", color="element")
    fig.update_xaxes(nticks=XTICKS)
    fig.update_layout(title_text="Processing Time")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Processing Time (ms)", nticks=YTICKS)
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
    df['timedelta'] = pd.to_timedelta(df['s_scheduletime'])
    df['scheduletime'] = df['timedelta'].dt.total_seconds() * 1000
    df['time'] = pd.to_datetime(df['s_time'])
    fig = px.line(df, x="time", y="scheduletime", color="pad")
    fig.update_xaxes(nticks=XTICKS)
    fig.update_layout(title_text="Schedule Time")
    fig.update_xaxes(title_text="Time (s)", nticks=XTICKS)
    fig.update_yaxes(title_text="Schedule Time (ms)", nticks=YTICKS)
    return fig


def plot_interactive_svg(html_file, dot_path):
    output_name = Path(f'{dot_path.parent / dot_path.stem}.svg')
    dot_to_svg_command = DOT_TO_SVG_COMMAND_TEMPLATE.format(dot_path=dot_path, output_name=output_name)

    subprocess.run(dot_to_svg_command, shell=True, check=True)

    if not output_name.exists():
        raise FileNotFoundError(f"Expected to find {output_name}")

    file_content = Path(output_name).read_text()
    pattern = r'<g id="graph0".*?</svg>'
    match = re.search(pattern, file_content, re.DOTALL)

    if not match:
        raise ValueError(f"Got invalid file {dot_path}")

    svg_content = match.group()
    svg_content = svg_content.replace('</svg>', '')
    svg_content_formatted = DOT_TEMPLATE.format(svg_content=svg_content)

    html_file.write(svg_content_formatted)


def plot_copyable_gst_launch(html_file, gst_launch_string):
    gst_launch_formatted = GST_LAUNCH_TEMPLATE.format(launch_pipeline=gst_launch_string)
    html_file.write(gst_launch_formatted)


def plot_svg_and_pipeline(html_file, traces_dir, exclude_svg=False):
    dot_files = list(Path(traces_dir).rglob('*.dot'))
    gst_launch_log = Path(f"{traces_dir}/pipeline.log")

    if gst_launch_log.exists():
        plot_copyable_gst_launch(html_file=html_file, gst_launch_string=gst_launch_log.read_text())

    # If the user included graph tracer and only one dot file found (we are expecting one)
    if not exclude_svg and len(dot_files) == 1:
        plot_interactive_svg(html_file, dot_files[0])


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
        'bufferdrop.log': plot_bufferdrop,
    }

    parser = argparse.ArgumentParser(description='Plot traces')
    parser.add_argument('-p', '--path', type=str, help='Path to traces dir')
    parser.add_argument('--exclude-svg', action='store_true', help='Do not include the svg in the HTML output')
    args = parser.parse_args()
    traces_dir = args.path
    exclude_svg = args.exclude_svg

    trace_log_files = list(Path(traces_dir).rglob('*.log'))

    html_output_path = Path(f"{traces_dir}/graphs.html")
    html_output_path.unlink(missing_ok=True)

    with html_output_path.open(mode='w') as f:
        plot_svg_and_pipeline(html_file=f, traces_dir=traces_dir, exclude_svg=exclude_svg)

        for trace_log_file in trace_log_files:
            if trace_log_file.name in tracer_to_plotter.keys():
                f.write(tracer_to_plotter[trace_log_file.name](
                    f"{traces_dir}/{trace_log_file.name}").to_html(full_html=False, include_plotlyjs='cdn'))

    print(f"{traces_dir}/graphs.html created")


if __name__ == "__main__":
    main()
