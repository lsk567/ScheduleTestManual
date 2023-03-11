rm -r build
mkdir build
cd build
# FIXME: Make sure to remove a few hard-coded lines in CMakeLists.txt. Peter thinks they should not be there.
# FIXME: Why does this configuration deadlock? Could it be the optimization?
# cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_INFO -DCMAKE_BUILD_TYPE=Release ..
# The DEBUG version works.
cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_INFO -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_ALL -DCMAKE_BUILD_TYPE=Debug ..
make
