### perf (great for multithreaded program)
sudo perf script > perf.script
# gprof2dot is not good at visualizing multithreaded data.
# The numbers might be off. But it's good to have the percentages.
sudo perf script | c++filt | gprof2dot -f perf | dot -Tpng -Gdpi=300 -o perf.png

### gprof (great for single-thread, but not very good multithreaded program)
## First, make sure that the -pg flags are set in rebuild.sh.
# gprof build/ScheduleTest gmon.out > perf.txt
# gprof2dot -f prof --node-label=self-time --node-label=self-time-percentage --node-label=total-time --node-label=total-time-percentage perf.txt -o perf.dot
# dot -Tpng -Gdpi=300 -o perf.png perf.dot
