from enum import Enum, auto


class ValueType(Enum):
    """
    There are two types of TAPPAS-tracers:
    - The one outputs numeric value, such as: QueueLevel, FrameRate.
    - The seconds output time value, such as: ProcessingTime
    
    This enum represent the types of the tracers  
    """
    Numeric = auto()
    Time = auto()


class TraceType(Enum):
    """
    An enum that maps between the supported tracers and their GST-Shark log file name
    """
    QueueLevel = 'queuelevel'
    ProcessingTime = 'proctime'
    FrameRate = 'framerate'

    def __str__(self):
        return self.value

    @classmethod
    def has_value(cls, value):
        """
        Based on: https://stackoverflow.com/a/43634746/5708016
        """
        return value in cls._value2member_map_
