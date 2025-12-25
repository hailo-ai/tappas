#!/usr/bin/env python
import os
from setuptools import setup, find_packages


def main():
    # Get version from environment variable or use default
    version = os.environ.get('TAPPAS_VERSION', '1.0.0')
    
    setup(
        name='hailo-tappas-core-python-binding',
        version=version,
        description='Python binding for tappas',
        long_description=' ',
        url='https://hailo.ai/',
        author='Hailo team',
        author_email='contact@hailo.ai',
        packages=find_packages(),
        install_requires=[],
        zip_safe=False,
        package_data={
            '': ['../*.so', 'gsthailo/*.py'],  # Includes .so files in the root and .py files in gsthailo directory
        },
        include_package_data=True
    )

if __name__ == '__main__':
    main()
