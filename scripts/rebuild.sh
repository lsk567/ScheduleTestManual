rm -r build
mkdir build
cd build

# FIXME: the RELEASE build type seems to be fine with perf?
# FIXME: Make sure to remove a few hard-coded lines in CMakeLists.txt. Peter thinks they should not be there.
cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_INFO -DCMAKE_BUILD_TYPE=Release ..
# cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_INFO -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_ALL -DCMAKE_BUILD_TYPE=Debug ..

# LF Tracing
# cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_INFO -DLF_TRACE=null -DCMAKE_BUILD_TYPE=Release ..

# Profiling using -pg
# cmake -DCMAKE_C_FLAGS=-pg -DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=2 -DSCHEDULER=FS -DLOG_LEVEL=LOG_LEVEL_INFO -DCMAKE_BUILD_TYPE=Debug ..

make
