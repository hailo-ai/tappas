from config import config
from pydantic.v1 import BaseModel, validator
from typing import List, Dict, Optional


class Bucket(BaseModel):
    url: str


class Buckets(BaseModel):
    buckets: Dict[str, Bucket]


class Requirement(BaseModel):
    bucket: str
    source: str
    destination: str
    should_extract: Optional[bool]

    @validator('source', allow_reuse=True)
    @validator('destination', allow_reuse=True)
    def replace_reserved_keywords_in_paths(cls, value):
        for keyword, value_to_replace_with in config.RESERVED_DOWNLOADER_KEYWORDS.items():
            if keyword in value:
                value = value.replace(keyword, value_to_replace_with)

        return value

    def __hash__(self):
        return hash(self.source)


class FolderRequirements(BaseModel):
    path: str
    requirements: List[Requirement] = []
