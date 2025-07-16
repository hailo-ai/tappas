from typing import List
from pydantic.v1 import BaseModel



class StrToReplace(BaseModel):
    file: str
    src: str
    to: str

class RegexRules(BaseModel):
    file: str
    src: str
    to: str
    num_of_appears: int


class ReleaseRules(BaseModel):
    rm_dirs : List[str] = None
    rm_files: List[str] = None
    str_replacement: List[StrToReplace] = None
    regex_rules: List[RegexRules] = None