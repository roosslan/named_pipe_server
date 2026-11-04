cmake_minimum_required(VERSION 3.20)
project(bghelper_runtime_tests LANGUAGES CXX)
enable_testing()
add_executable(pipe_runtime_tests pipe_runtime_tests.cpp
    ../cnamedps/CEvent.cpp ../cnamedps/CHandle.cpp ../cnamedps/CIOBuffer.cpp
    ../cnamedps/CIOContext.cpp ../cnamedps/CNamedPipeServer.cpp
    ../cnamedps/CNamedPipeWorkerPool.cpp ../cnamedps/Exception.cpp ../cnamedps/String.cpp)
# Build the actual pipe implementation with a minimal precompiled-header substitute;
# these tests need the Windows SDK, but do not require MFC or sensitive_data.h.
target_include_directories(pipe_runtime_tests PRIVATE support ../cnamedps)
target_compile_features(pipe_runtime_tests PRIVATE cxx_std_20)
target_compile_definitions(pipe_runtime_tests PRIVATE _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN)
target_compile_options(pipe_runtime_tests PRIVATE /utf-8 /W4)
add_test(NAME pipe_runtime COMMAND pipe_runtime_tests)
set_tests_properties(pipe_runtime PROPERTIES TIMEOUT 45)
add_executable(socket_runtime_tests socket_runtime_tests.cpp)
target_include_directories(socket_runtime_tests PRIVATE ../src)
target_compile_features(socket_runtime_tests PRIVATE cxx_std_20)
target_compile_definitions(socket_runtime_tests PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_options(socket_runtime_tests PRIVATE /utf-8 /W4)
target_link_libraries(socket_runtime_tests PRIVATE ws2_32)
add_test(NAME socket_runtime COMMAND socket_runtime_tests)
set_tests_properties(socket_runtime PROPERTIES TIMEOUT 15)
