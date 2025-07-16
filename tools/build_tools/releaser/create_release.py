#!/usr/bin/env python3
import argparse
import logging
import os
import re
import shutil
from pathlib import Path

import yaml

import sys
sys.path.append(os.path.dirname(os.path.dirname(__file__)))


from release_rules_model import ReleaseRules

RULES_FILES_PATH = Path(__file__).absolute().parent / "release_manipulations_rules.yaml"

DOCKER_BASE_PATH = "docker/Dockerfile.tappas_base"
DOCKER_TAPPAS_PATH = "docker/Dockerfile.tappas"
DOCKER_COMBINED_PATH = "docker/Dockerfile.tappas_combined"

DOCKER_COMPILATION_FILES = ["build_docker.sh",
                            "build_docker_cross_compile.sh",
                            "docker"]

class NotFoundException(Exception):
    pass


class FailedToChangeException(Exception):
    pass


class CmdLineArgsException(Exception):
    pass


def rm_directories(tappas_path, dirs_to_remove):
    for dir in dirs_to_remove:
        path = tappas_path / dir
        logging.info(f"Removing directory: {path}")
        shutil.rmtree(path)


def rm_files(tappas_path, files_to_remove):
    for file in files_to_remove:
        path = tappas_path / file
        logging.info(f"Removing file: {path}")
        os.remove(path)


def replace_string(tappas_path, rules):
    for replace_rule in rules:
        file_path = tappas_path / replace_rule.file
        file_data = file_path.read_text()
        if replace_rule.src not in file_data:
            raise NotFoundException(f"{replace_rule.src} not found in {file_path}")

        file_data = file_data.replace(replace_rule.src, replace_rule.to)

        if replace_rule.src in file_data:
            raise FailedToChangeException(f"{replace_rule.src} not replaced in {file_path}")

        logging.info(f'Replacing {replace_rule.src} to {replace_rule.to} in file {file_path}')
        file_path.write_text(file_data)


def apply_regex_replacement_rules(tappas_path, regex_rules):
    for rule in regex_rules:
        file_path = tappas_path / rule.file
        logging.info(f"Replacing regex in file: {file_path}")
        file_data = file_path.read_text()
        num_of_appears = len(re.findall(rule.src, file_data))

        if num_of_appears != rule.num_of_appears:
            NotFoundException(f"expected {rule.num_of_appears} appears of regex in file, but {num_of_appears} founds.")

        pattern = re.compile(rule.src)
        file_data = pattern.sub(rule.to, file_data)
        file_path.write_text(file_data)


def _load_rules():
    yaml_rules = RULES_FILES_PATH.read_text()
    rules = yaml.safe_load(yaml_rules)
    rules = ReleaseRules.parse_obj(rules)
    return rules

def create_combined_docker(tappas_path):    
    tappas_base_text = (tappas_path / DOCKER_BASE_PATH).read_text().splitlines()
    tappas_text_without_from = (tappas_path / DOCKER_TAPPAS_PATH).read_text().splitlines()[2:]
    (tappas_path / DOCKER_COMBINED_PATH).write_text("\n".join(tappas_base_text + tappas_text_without_from))

def move_docker_files_from_release(tappas_path):
    for copy_src in DOCKER_COMPILATION_FILES:
        absolute_src = (tappas_path / copy_src).absolute()
        absolute_dest = absolute_src.parent.parent / copy_src
        is_dir = absolute_src.is_dir()
        
        # Chose copy funciton
        if is_dir:
            copy_function=shutil.copytree
        else:
            copy_function=shutil.copy2
        
        # Check if file/dir exist, if so erase it
        if absolute_dest.exists():
            if is_dir:
                shutil.rmtree(str(absolute_dest))
            else:
                absolute_dest.unlink()

        shutil.move(str(absolute_src), str(absolute_dest), copy_function=copy_function)


def main(tappas_path):
    rules = _load_rules()
    if rules.rm_dirs:
        logging.info("Removing directories")
        rm_directories(tappas_path, rules.rm_dirs)
    if rules.rm_files:
        logging.info("Removing files")
        rm_files(tappas_path, rules.rm_files)
    if rules.str_replacement:
        logging.info("Applying string replacement rules")
        replace_string(tappas_path, rules.str_replacement)
    if rules.regex_rules:
        logging.info("Applying regex replacement rules")
        apply_regex_replacement_rules(tappas_path, rules.regex_rules)
    create_combined_docker(tappas_path)
    move_docker_files_from_release(tappas_path)
    logging.info("Success to creating release")


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(levelname)s:%(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--path', help='Path to TAPPAS repo to modify (files would be modified and deleted there)',
                        type=str, required=True)
    args = parser.parse_args()

    main(tappas_path=Path(args.path))
