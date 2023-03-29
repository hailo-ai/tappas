import argparse
from pathlib import Path
from typing import List, Optional

from dot_visualizer.tracers import get_trace_managers

from dot_visualizer.enums import TraceType
from dot_visualizer.graph_manager import GraphManager


def get_tracers_logs_found_and_supported(tracers_path: Path) -> List[TraceType]:
    """
    Returns a list of tracer that are both supported and have a log file available
    :param tracers_path: Path to the tracers directory
    """
    tracers_logs_found = tracers_path.glob("*.log")
    tracers_logs_found_and_supported = [TraceType(tracer_log.stem) for tracer_log in tracers_logs_found
                                        if TraceType.has_value(tracer_log.stem)]

    return tracers_logs_found_and_supported


def get_graph_file_path(tracers_path: Path):
    """
    Assuming that the tracers_path has a single dot file with the following schema: pipeline_<date>.dot.
    Extract and return the path to the dot file
    :param tracers_path: Path to the tracers directory
    """
    graph_file = list(tracers_path.glob("pipeline*.dot"))

    if len(graph_file) != 1:
        raise ValueError(f"Expected exactly one pipeline.dot file, found {len(graph_file)}")

    return graph_file[0]


def construct_graph_label(tracers_logs_found_and_supported: List[TraceType],
                          tracer_for_paint: Optional[TraceType]) -> str:
    """
    Construct and return a label for the graph (Display name)
    :param tracers_logs_found_and_supported: List[TracerType]: List of tracers found and supported
    :param tracer_for_paint: Optional[TracerType]: A link to the trace we are using to paint accordingly
    """
    label = "TAPPAS Adjusted DOT File"
    label += "\n Tracers:" + ' '.join([str(trace) for trace in tracers_logs_found_and_supported])

    if tracer_for_paint:
        label += "\n" + f"Painting based on {tracer_for_paint} Tracer"

    return label


def handle_graph_adjustments(graph_manager: GraphManager, tracers_path: Path, tracer_for_paint: TraceType,
                             tracers_logs_found_and_supported: List[TraceType]):
    """
    Handle the color and label adjustments of the elements in the graph
    """
    trace_manager_by_type = get_trace_managers(graph_manager=graph_manager, tracers_path=tracers_path)

    if tracer_for_paint:
        if tracer_for_paint not in tracers_logs_found_and_supported:
            raise ValueError(f"Tracer {tracer_for_paint} was chosen for painting, but no tracer-log could be found")

        trace_manager_by_type[tracer_for_paint].adjust_graph_color()

    for tracer in tracers_logs_found_and_supported:
        trace_manager_by_type[tracer].adjust_graph_labels()


def init_argparse():
    parser = argparse.ArgumentParser()
    parser.add_argument('path', help='Path to the tracers dir', type=Path)
    parser.add_argument('--tracer-for-paint', help='Name of the tracer to paint the elements relative to',
                        default=None, type=TraceType, choices=list(TraceType))

    args = parser.parse_args()

    return args


def entry():
    args = init_argparse()
    tracers_path, tracer_for_paint = args.path, args.tracer_for_paint

    graph_file_path = get_graph_file_path(tracers_path)
    graph_manager = GraphManager(file_path=graph_file_path)
    tracers_logs_found_and_supported = get_tracers_logs_found_and_supported(tracers_path)

    handle_graph_adjustments(graph_manager=graph_manager, tracers_path=tracers_path, tracer_for_paint=tracer_for_paint,
                             tracers_logs_found_and_supported=tracers_logs_found_and_supported)

    label = construct_graph_label(tracers_logs_found_and_supported, tracer_for_paint)
    graph_manager.change_label(label)

    output_graph_path = tracers_path / 'output_pipeline.dot'
    graph_manager.save(output_path=output_graph_path)
    print(f"Graph has been generated successfully {output_graph_path}")


if __name__ == '__main__':
    entry()
