rm -r build
mkdir build
cd build
# FIXME: Make sure to remove a few hard-coded lines in CMakeLists.txt. Peter thinks they should not be there.
cmake -DLF_REACTION_GRAPH_BREADTH=3 -DLF_THREADED=1 -DNUMBER_OF_WORKERS=0 -DSCHEDULER=FS -DLOG_LEVEL=2 -DCMAKE_BUILD_TYPE=Release ..
make
