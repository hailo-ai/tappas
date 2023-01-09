import os
import re
from pathlib import Path

from setuptools import setup, find_packages

# Complicated line to extract RELEASE 20.04/21.04/22.04
lsb_release = list(filter(lambda x: 'RELEASE' in x,
                          Path('/etc/lsb-release').read_text().split('\n')))[0].split('=')[1].replace('.', '_')

required = Path(f'requirements_{lsb_release}.txt').read_text().splitlines()


def get_tappas_release_version():
    tappas_repository_root = os.path.realpath(os.path.join(os.path.dirname(__file__), '../..'))
    content = Path(f"{tappas_repository_root}/core/hailo/meson.build").read_text()
    tappas_version_raw = next(line for line in content.split('\n') if "version :" in line)
    tappas_version = re.search("([0-9]+.[0-9]+.[0-9]+)", tappas_version_raw).group(1)

    return tappas_version


setup(
    name="hailo-tappas-run-apps",
    version=get_tappas_release_version(),
    packages=find_packages(),
    install_requires=required,
    entry_points={
        'console_scripts': [
            'hailo_run_app = main:entry',
        ],
    },
)
