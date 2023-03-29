#!/usr/bin/env python
# PYTHON_ARGCOMPLETE_OK
import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional, List, Dict

import argcomplete
from dataclasses import dataclass


@dataclass
class Argument:
    names: List[str]
    help_text: str
    boolean: bool = True
    choices: Optional[List[str]] = None


@dataclass
class App:
    name: str
    comment: Optional[str] = ''


@dataclass
class Task:
    name: str
    apps: List[App]
    comment: Optional[str] = ''


TAPPAS_PATH = Path(__file__).resolve().parent.parent.parent
APPS_PATH = TAPPAS_PATH / "apps" / "h8" / "gstreamer" / "general"


def get_tasks() -> Dict[str, Task]:
    tasks = dict()
    shell_apps = APPS_PATH.parent.rglob("general/*/*.sh")

    for app in shell_apps:
        if app.parent.stem not in tasks:

            # Don't store the sanity pipeline
            if app.parent.stem == 'sanity_pipeline':
                continue

            tasks[app.parent.stem] = Task(name=app.parent.stem, apps=list())

        tasks[app.parent.stem].apps.append(App(name=app.stem))

    return tasks


def get_app_help(app_path):
    app_help_subprocess = subprocess.run(f'{app_path} --help', check=True, shell=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
    help_splitted_by_lines = app_help_subprocess.stdout.decode().split('\n')
    help_splitted_by_lines = [line for line in help_splitted_by_lines if line.strip()]

    return help_splitted_by_lines


def extract_choices(argument_help, is_argument_boolean):
    argument_choices = None
    brackets = ['[', ']']

    # Checks if the argument might have choices
    # A valid choices would look like [a,b,c,d]
    if not is_argument_boolean and all(bracket in argument_help for bracket in brackets):
        choices_start = argument_help.index('[')
        choices_end = argument_help.index(']')

        # Add one become the bracket is one char long
        argument_choices = argument_help[choices_start + 1:choices_end].split(',')
        argument_choices = [arg.strip() for arg in argument_choices]

    return argument_choices


def get_arguments_from_app(app_path):
    arguments = list()
    help_splitted_by_lines = get_app_help(app_path)

    for line in help_splitted_by_lines:
        words_in_line = line.split()
        is_argument_boolean = True

        if_line_is_not_parsable = not words_in_line[0].startswith('-')
        if_reserved_argparse_flags = '-h' in words_in_line or '--help' in words_in_line

        if if_line_is_not_parsable or if_reserved_argparse_flags:
            continue

        argument_names = [word for word in words_in_line if word.startswith('-')]

        for argument_name in argument_names:
            name_index = words_in_line.index(argument_name)

            # The standard is that if the upper word is capital, this argument is not boolean
            if words_in_line[name_index + 1].isupper():
                is_argument_boolean = False
                del words_in_line[name_index + 1]

        argument_help = ' '.join(words_in_line[words_in_line.index(argument_names[-1]) + 1:])
        argument_choices = extract_choices(argument_help=argument_help, is_argument_boolean=is_argument_boolean)

        arguments.append(Argument(names=argument_names, help_text=argument_help, boolean=is_argument_boolean,
                                  choices=argument_choices))

    return arguments


def add_app_to_parser(app: App, task: Task, parser: argparse.ArgumentParser):
    app_path = APPS_PATH / task.name / (app.name + ".sh")
    apps_arguments = get_arguments_from_app(app_path)

    for argument in apps_arguments:
        kwargs = dict()

        # Argparse doesn't accept choices as None
        if argument.choices:
            kwargs['choices'] = argument.choices

        parser.add_argument(*argument.names, help=argument.help_text,
                            action='store_true' if argument.boolean else None,
                            **kwargs)


def build_argparse(tasks):
    parser = argparse.ArgumentParser()
    tasks_sub_parsers = parser.add_subparsers(help='tasks help')

    for task in tasks.values():
        task_parser = tasks_sub_parsers.add_parser(name=task.name, help=task.comment)

        if len(task.apps) == 1:
            add_app_to_parser(app=task.apps[0], task=task, parser=task_parser)
        else:
            app_sub_parser = task_parser.add_subparsers(help='App helper')

            for app in task.apps:
                app_parser = app_sub_parser.add_parser(name=app.name, help=app.comment)
                add_app_to_parser(app=app, task=task, parser=app_parser)

    return parser


def run_app(tasks):
    task_folder = APPS_PATH / sys.argv[1]
    tasks_with_single_app_names = [task_name for (task_name, task) in tasks.items() if len(task.apps) == 1]

    # The first item is the script name
    does_task_with_single_app_selected = sys.argv[1] in tasks_with_single_app_names

    if does_task_with_single_app_selected:
        if len(sys.argv) < 2:
            raise TypeError("Expected more argv")

        app_path = list(task_folder.glob("*.sh"))
        app_path = app_path[0]
        app_flags = ' '.join(sys.argv[2:])
    else:
        if len(sys.argv) < 3:
            raise TypeError("Expected more argv")

        app_path = task_folder / (sys.argv[2] + ".sh")
        app_flags = ' '.join(sys.argv[3:])

    run_app_command = f"{app_path} {app_flags}"
    subprocess.run(run_app_command, shell=True)


def entry():
    tasks = get_tasks()
    parser = build_argparse(tasks)
    argcomplete.autocomplete(parser)
    parser.parse_args()

    try:
        run_app(tasks)
    except (TypeError, IndexError):
        parser.print_help()


if __name__ == '__main__':
    entry()
