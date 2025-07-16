Using record_perf script to get records from tappas
----------------------------------------------------
1) Enter core/hailo/meson.build

2) Add to the Compiler Arguments:

    .. code-block:: sh

        add_global_arguments('-ggdb', language : 'cpp')

3) Compile tappas in debug mode

4) Run:

    .. code-block:: sh

        ./scripts/misc/internals/record_perf/record_perf.sh <time>

5) run specific pipeline you want to debug

6) upload the output file to https://flamegraph.com/