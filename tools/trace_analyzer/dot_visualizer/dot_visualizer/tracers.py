import abc
from pathlib import Path
from typing import Dict

from dot_visualizer.trace_manager import TraceManager

from dot_visualizer.enums import TraceType, ValueType
from dot_visualizer.graph_manager import GraphManager


class BaseTracer(abc.ABC):
    def __init__(self, trace_type, columns_to_keep, value_type, label, reverse=False):
        self.trace_type = trace_type
        self._columns_to_keep = columns_to_keep
        self._value_type = value_type
        self._label = label
        self._reverse = reverse

    def generate_trace_manager(self, graph_manager: GraphManager, tracers_path: Path):
        trace_manager = TraceManager(graph_manager=graph_manager,
                                     log_file_path=tracers_path / (str(self.trace_type.value) + ".log"),
                                     columns_to_keep=self._columns_to_keep,
                                     value_type=self._value_type, label=self._label, reverse=self._reverse)

        return trace_manager


class QueueLevelTracer(BaseTracer):
    COLUMNS_TO_KEEP = {0: 'timestamp', 7: 'element_name', 10: 'value'}
    LABEL = 'Queue level'

    def __init__(self):
        super().__init__(trace_type=TraceType.QueueLevel, columns_to_keep=self.COLUMNS_TO_KEEP,
                         value_type=ValueType.Numeric, label=self.LABEL)


class FramerateTracer(BaseTracer):
    COLUMNS_TO_KEEP = {0: 'timestamp', 7: 'element_name', 8: 'value'}
    LABEL = 'FPS'

    def __init__(self):
        super().__init__(trace_type=TraceType.FrameRate, columns_to_keep=self.COLUMNS_TO_KEEP,
                         value_type=ValueType.Numeric, label=self.LABEL)


class ProcessingTimeTracer(BaseTracer):
    COLUMNS_TO_KEEP = {0: 'timestamp', 7: 'element_name', 8: 'value'}
    LABEL = 'Proc time (ms)'

    def __init__(self):
        super().__init__(trace_type=TraceType.ProcessingTime, columns_to_keep=self.COLUMNS_TO_KEEP,
                         value_type=ValueType.Time, label=self.LABEL, reverse=True)


def get_trace_managers(graph_manager: GraphManager, tracers_path: Path) -> Dict[TraceType, TraceManager]:
    """
    Initialize and return a dict of: Dict[TracerType, TracerManager]
    """
    trace_manager_by_type = dict()
    tracers_classes = [QueueLevelTracer, ProcessingTimeTracer, FramerateTracer]

    for tracer_class in tracers_classes:
        tracer_instance = tracer_class()
        trace_manager_by_type[tracer_instance.trace_type] = tracer_instance.generate_trace_manager(graph_manager,
                                                                                                   tracers_path)

    return trace_manager_by_type
