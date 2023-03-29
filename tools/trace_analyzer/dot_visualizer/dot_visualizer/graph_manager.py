import math
import pydot
import re
from colour import Color
from pathlib import Path

from typing import Dict

def generate_colors(num_of_colors=10):
    return [color.hex_l for color in Color("red").range_to(Color("green"), steps=num_of_colors)]


class GraphElement:
    """
    A class that is responsible for editing an element in the graph
    """

    def __init__(self, element_dict):
        self._element_dict = element_dict

    def change_color(self, new_hex_color):
        self._element_dict['attributes']['penwidth'] = '3.0'
        self._element_dict['attributes']['fillcolor'] = new_hex_color

    def append_text(self, new_text):
        self._element_dict['attributes']['label'] = f'" {new_text} \\n' + \
                                                    self._element_dict['attributes']['label'][1:]


class GraphManager:
    """
    Load, save and modify the text and color of a graph
    """
    EXTRACT_ELEMENT_NAME_REGEX = 'cluster_(.+)_0x'

    def __init__(self, file_path: Path):
        self._file_path = file_path
        self._graph = None
        self._element_to_graph_mapping = dict()

        self._graph = self.load(file_path)
        self._colors = generate_colors()
        self._fix_graph_broken_legend()
        self._prepare_element_mapping(graph=self._graph.obj_dict)

    def load(self, file_path: str) -> pydot.Graph:
        dot_file_path = Path(file_path)
        graphs = pydot.graph_from_dot_file(dot_file_path, encoding="utf-8")

        return graphs[0]

    def save(self, output_path):
        # List of outputs format supported could be found here:
        # https://graphviz.org/docs/outputs/
        output_path_format = Path(output_path).suffix

        if output_path_format == ".dot":
            self._graph.write_dot(output_path, encoding="utf-8")
        elif output_path_format == ".pdf":
            self._graph.write_pdf(output_path, encoding="utf-8")
        else:
            raise TypeError(f"Format is not supported - {output_path_format}")

    def change_label(self, new_label):
        if self._graph:
            self._graph.obj_dict['attributes']['label'] = f'"{new_label}"'

    def _value_to_hex_color(self, element_name, value_elements_and_index_map, reverse):
        # Calculates the color of the element, For the example:
        # we have an that the element value is ranked in place 5/20 element, And we have 8 colors.
        # index = 8 * (5/20) = 2
        color_index = len(self._colors) * (value_elements_and_index_map[element_name] /
                                           len(value_elements_and_index_map))

        # Index exceed if not subtracted by one in that case
        if int(math.floor(color_index)) == len(self._colors):
            color_index -= 1

        if reverse:
            color_index = (len(self._colors) - 1) - color_index

        color = self._colors[math.floor(color_index)]

        return color

    def _fix_graph_broken_legend(self):
        """
        Gst-shark breaks the `legend`, this function fixes it
        """
        if '\n' in self._graph.obj_dict['nodes']['legend'][0]['attributes']:
            del self._graph.obj_dict['nodes']['legend'][0]['attributes']['\n']

    def _prepare_element_mapping(self, graph):
        """
        Create a mapping between the element name to the element object.
        Some elements might be nested, so recursive is needed
        """
        for key, value in graph['subgraphs'].items():
            if key.endswith('src') or key.endswith('sink'):
                continue

            if len(value) != 1:
                raise TypeError('Broken graph')

            element_name = re.search(pattern=self.EXTRACT_ELEMENT_NAME_REGEX, string=key).groups(0)[0]
            self._element_to_graph_mapping[element_name] = value[0]

            self._prepare_element_mapping(value[0])

    def adjust_graph_labels(self, mean_value_by_element: Dict[str, float], label: str):
        """
        Loop through all the element in the `mean_value_by_element` dict and modify their text
        :param mean_value_by_element: Dict of: {Element Name: Element Mean value}, calculated by `TraceParser`
        :param label: The prefix to the text (AKA label)
        """
        for element_name, element_graph in self._element_to_graph_mapping.items():
            if element_name in mean_value_by_element:
                value = mean_value_by_element[element_name]

                element = GraphElement(element_dict=element_graph)
                element.append_text(f"{label}: {value:.2f}")

    def adjust_graph_colors(self, mean_value_by_element, value_elements_and_index_map, reverse: bool = False):
        """
        Loop through all the element in the `mean_value_by_element` dict and modify their color
        :param mean_value_by_element: Dict of: {Element Name: Element Mean value}, calculated by `TraceParser`
        :param value_elements_and_index_map: Dict of elements and their index, calculated by `TraceParser`
        :param reverse: Should the coloring order be reversed
        """
        for element_name, element_graph in self._element_to_graph_mapping.items():
            if element_name in mean_value_by_element:
                element = GraphElement(element_dict=element_graph)
                element.change_color(self._value_to_hex_color(element_name, value_elements_and_index_map,
                                                              reverse=reverse))
