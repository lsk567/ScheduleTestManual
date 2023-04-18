# First, make sure that the -pg flags are set in rebuild.sh.
gprof build/ScheduleTest gmon.out > perf.txt
gprof2dot -f prof perf.txt -o perf.dot
dot -Tpng -Gdpi=300 -o perf.png perf.dot
