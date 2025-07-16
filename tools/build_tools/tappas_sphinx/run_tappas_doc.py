#!/usr/bin/env python3

from build_doc import TappasDocBuilder
from build_tool.builder_engine import BuilderEngine, BuildFailedException


def main():
    tappas_doc_comp = TappasDocBuilder()
    builder_engine = BuilderEngine([tappas_doc_comp])
    builder_engine.build_locally()
    if not builder_engine.is_all_builds_successful():
        raise BuildFailedException("One or more of the SW components failed to build. Please see details in log above.")

if __name__ == '__main__':
    main()
