#!/bin/bash
# Based on top of https://github.com/cyrus-and/gdb-dashboard
set -e
wget -P ~ https://git.io/.gdbinit
pip install pygments