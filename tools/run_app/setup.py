from pathlib import Path

from setuptools import setup, find_packages

# Complicated line to extract RELEASE 18.04/20.04
lsb_release = list(filter(lambda x: 'RELEASE' in x,
                          Path('/etc/lsb-release').read_text().split('\n')))[0].split('=')[1].replace('.', '_')

required = Path(f'requirements_{lsb_release}.txt').read_text().splitlines()

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
