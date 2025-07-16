#!/usr/bin/env python
"""
Create desktop links for Linux based OS
https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

Run this script with "--camera-device /dev/videoX" flag, and then refresh the desktop to mark the shortcuts as trusted
"""
import argparse
import json
import subprocess
from pathlib import Path

desktop_entry_template = """
[Desktop Entry]
Version={version}
Name={name}
Comment={comment}
Exec=bash -c '{exec_path}'
Icon={icon}
Terminal=true
Type=Application
Categories=Application;
"""

REPO_PATH = Path(__file__).resolve().parent.parent.parent
DOCKER_BASE_APPS_PATH = Path("/local/workspace/tappas/apps/h8/gstreamer/general")
RUN_APP_TEMPLATE = """{tappas_path}/docker/run_docker.sh --resume --resume-command "{command}" """
ICON_PATH = REPO_PATH / "resources" / "hailo_logo.png"
APPS_CONFIG_PATH = Path(__file__).parent / 'config.json'
DEFAULT_CAMERA_DEVICE = "/dev/video2"


class App:
    def __init__(self, name: str, exec_command: str, flags='', version="1.0", comment="Hailo App"):
        self.name = name
        self.exec_path = exec_command
        self.flags = flags
        self.version = version
        self.comment = comment


def mark_desktop_file_as_trusted(desktop_entry_path):
    shell_cmd = f"gio set {desktop_entry_path} metadata::trusted yes"
    subprocess.run(shell_cmd.split(), check=True)


def generate_desktop_file(app: App):
    desktop_entry = desktop_entry_template.format(name=app.name, version=app.version, comment=app.comment,
                                                  exec_path=app.exec_path, icon=ICON_PATH)

    desktop_entry_path = Path().home() / "Desktop" / (app.name + ".desktop")

    if desktop_entry_path.exists():
        desktop_entry_path.unlink()

    desktop_entry_path.touch(mode=0o775)
    desktop_entry_path.write_text(desktop_entry)

    mark_desktop_file_as_trusted(desktop_entry_path)


def get_apps_list(camera_device=None):
    all_apps = list()
    apps_config_raw = Path(APPS_CONFIG_PATH).read_text()
    apps_config_loaded = json.loads(apps_config_raw)

    for app_dir, apps in apps_config_loaded.items():
        for app_name, app in apps.items():
            app_full_path = DOCKER_BASE_APPS_PATH / app_dir / app['path']
            flags = app.get('flags', '')

            # Replace the camera device if requested
            if camera_device and DEFAULT_CAMERA_DEVICE in flags:
                flags = flags.replace(DEFAULT_CAMERA_DEVICE, camera_device)

            command_with_flags = f"{app_full_path} {flags}"
            run_app_full_command = RUN_APP_TEMPLATE.format(tappas_path=REPO_PATH, command=command_with_flags)

            app = App(name=app_name, exec_command=run_app_full_command)
            all_apps.append(app)

    return all_apps


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--camera-device', help=f'Which camera device to use (default is {DEFAULT_CAMERA_DEVICE})',
                        type=str, required=False, default=None)
    args = parser.parse_args()

    return args


if __name__ == '__main__':
    args = parse_args()
    apps = get_apps_list(camera_device=args.camera_device)

    for app in apps:
        generate_desktop_file(app)

    print('Done creating the desktop entry files, please refresh (pressing F5)')
