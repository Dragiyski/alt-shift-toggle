cmake_minimum_required(VERSION 3.0)
project(alt_shift_notify NONE)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

execute_process(COMMAND "${Python3_EXECUTABLE}" "-m" "venv" "venv" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

configure_file("${CMAKE_SOURCE_DIR}/requirements.txt" "${CMAKE_BINARY_DIR}/requirements.txt" COPYONLY)

execute_process(COMMAND "${CMAKE_BINARY_DIR}/venv/bin/python" "-m" "pip" "install" "-r" "requirements.txt" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")