from typing import Dict

import pandas as pd

from dot_visualizer.enums import ValueType


class TraceParser:
    def __init__(self, log_file_path, columns_to_keep, value_type: ValueType):
        self._log_file_path = log_file_path
        self._columns_to_keep = columns_to_keep
        self._value_type = value_type

        self._df = pd.read_csv(log_file_path, sep=' ', header=None)
        self._df = self._arrange_columns_names(self._df)
        self._df = self._arrange_columns_values(self._df)

    def _arrange_columns_names(self, df):
        """
        Rename columns
        :param df: The dataframe to modify
        :return: The dataframe with the modified column names
        """
        df = df.iloc[:, list(self._columns_to_keep.keys())]
        df = df.rename(columns=self._columns_to_keep)

        return df

    def _arrange_columns_values(self, df):
        """
        Modify the element name, convert the value from numeric/time to pandas correct type
        :param df: the dataframe to modify
        :return: The modified dataframe
        """

        def _from_right_bracket_on(val):
            # The values in the tracers are formatted like this: "time=(string)123"
            # This functions converts them to --> "123"
            return val[val.rindex(')') + 1: len(val) - 1]

        df['element_name'] = df['element_name'].apply(_from_right_bracket_on)
        df['value'] = df['value'].apply(_from_right_bracket_on)

        if self._value_type == ValueType.Numeric:
            df['value'] = pd.to_numeric(df['value'])
        elif self._value_type == ValueType.Time:
            df['value'] = pd.to_timedelta(df['value'])
            df['value'] = df['value'].dt.total_seconds() * 1000
        else:
            raise TypeError(f'{self._value_type} is not supported yet')

        return df

    def get_mean_value_by_element(self) -> Dict[str, float]:
        """
        Calculate and return the mean value of each element
        :return: Dict of: {Element Name: Element Mean value}
        """

        def remove_src_from_element_name(val):
            if '_src' in val:
                return val[: val.index('_src')]

            return val

        df_without_src = self._df
        df_without_src['element_name'] = df_without_src['element_name'].apply(remove_src_from_element_name)

        df_grouped = df_without_src.groupby('element_name')
        dict_of_mean_values_per_element = df_grouped.mean()['value'].to_dict()

        return dict_of_mean_values_per_element

    def get_elements_and_index_map(self, mean_value_by_element=None):
        mean_value_by_element = mean_value_by_element or self.get_mean_value_by_element()

        # Get a list of elements (dict-key) sorted by fps (dict-value) in ascending order
        # F.E : {'a': '2', 'b': 1, 'c': 0} --> ['c', 'b', 'a']
        # Source of the sort method: https://stackoverflow.com/a/7340031/5708016
        elements_sorted_by_values = sorted(mean_value_by_element, key=mean_value_by_element.get)

        # Create a mapping between the element_name and the index in the sorted list
        # Continuing with the example above:  --> {'c': 0, 'b': 1, 'a': 2}
        element_and_index_map = {key: i for i, key in enumerate(elements_sorted_by_values)}

        return element_and_index_map
