from pathlib import Path

from setuptools import setup, find_packages

required = Path('requirements.txt').read_text().splitlines()

setup(
    name="hailo-run-apps",
    version="1.0",
    packages=find_packages(),
    install_requires=required,
    entry_points={
        'console_scripts': [
            'hailo_run_app = main:entry',
        ],
    },
)
