# CMake generated Testfile for 
# Source directory: /home/LinuxWork/project/c/LiveOrigin/service/lib/cJSON
# Build directory: /home/LinuxWork/project/c/LiveOrigin/service/cmake-build-debug/lib/cJSON
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(cJSON_test "/home/LinuxWork/project/c/LiveOrigin/service/cmake-build-debug/lib/cJSON/cJSON_test")
set_tests_properties(cJSON_test PROPERTIES  _BACKTRACE_TRIPLES "/home/LinuxWork/project/c/LiveOrigin/service/lib/cJSON/CMakeLists.txt;248;add_test;/home/LinuxWork/project/c/LiveOrigin/service/lib/cJSON/CMakeLists.txt;0;")
subdirs("tests")
subdirs("fuzzing")
