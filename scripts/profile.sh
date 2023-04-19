#!/usr/bin/env bash

set -euxo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/clean.sh
$SCRIPT_DIR/rebuild.sh
### perf (great for multithreaded program)
sudo perf record -g -- $SCRIPT_DIR/../build/ScheduleTest -o 1 minute
$SCRIPT_DIR/gen_report.sh
