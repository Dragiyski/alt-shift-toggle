cmake_minimum_required(VERSION 3.0)
project(kde_next_layout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

find_program(QDBUS_EXECUTABLE qdbus NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)

if(QDBUS_EXECUTABLE)
    message(STATUS "qdbus found: ${QDBUS_EXECUTABLE}")
else()
    message(FATAL_ERROR "qdbus not found")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(setcap_postinstall ${PROJECT_BINARY_DIR}/postinstall/setcap.cmake)
set(DESTINATION_EXECUTABLE ${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME})
configure_file(${CMAKE_SOURCE_DIR}/setcap.cmake.in ${setcap_postinstall} @ONLY)

install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION bin)
install(SCRIPT ${setcap_postinstall})

if(X11_INCLUDE_DIRS)
    target_include_directories(${CMAKE_PROJECT_NAME} ${X11_INCLUDE_DIRS})
endif(X11_INCLUDE_DIRS)
if(X11_LIBRARIES)
    target_link_libraries(${CMAKE_PROJECT_NAME} ${X11_LIBRARIES} Threads::Threads)
endif(X11_LIBRARIES)