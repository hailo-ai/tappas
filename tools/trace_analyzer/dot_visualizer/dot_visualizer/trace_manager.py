from dot_visualizer.trace_parser import TraceParser


class TraceManager:
    def __init__(self, graph_manager, log_file_path, columns_to_keep, value_type, label, reverse=False):
        self._graph_manager = graph_manager
        self._tracer_parser = TraceParser(log_file_path=log_file_path,
                                          columns_to_keep=columns_to_keep,
                                          value_type=value_type)
        self._label = label
        self._reverse = reverse

    def adjust_graph_labels(self):
        """
        Get the list of `mean value` for each element, and append them to the element text
        """
        mean_value_by_element = self._tracer_parser.get_mean_value_by_element()
        self._graph_manager.adjust_graph_labels(mean_value_by_element=mean_value_by_element, label=self._label)

    def adjust_graph_color(self):
        mean_value_by_element = self._tracer_parser.get_mean_value_by_element()
        elements_and_index_map = self._tracer_parser.get_elements_and_index_map(
            mean_value_by_element=mean_value_by_element)

        self._graph_manager.adjust_graph_colors(mean_value_by_element=mean_value_by_element,
                                                reverse=self._reverse,
                                                value_elements_and_index_map=elements_and_index_map)
