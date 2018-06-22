cmake_minimum_required(VERSION 3.0)
project(xkb_next_layout)

set(CMAKE_CXX_STANDARD 17)

add_executable(xkb_next_layout main.cpp)

find_package(X11 REQUIRED)

if (NOT X11_FOUND)
    message(ERROR Unable to find X11 library)
endif(NOT X11_FOUND)

set(setcap_postinstall ${PROJECT_BINARY_DIR}/postinstall/setcap.cmake)
set(DESTINATION_EXECUTABLE ${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME})
configure_file(${CMAKE_SOURCE_DIR}/setcap.cmake.in ${setcap_postinstall} @ONLY)

install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION bin)
install(SCRIPT ${setcap_postinstall})

if(X11_INCLUDE_DIRS)
    target_include_directories(${CMAKE_PROJECT_NAME} ${X11_INCLUDE_DIRS})
endif(X11_INCLUDE_DIRS)
if(X11_LIBRARIES)
    target_link_libraries(${CMAKE_PROJECT_NAME} ${X11_LIBRARIES})
endif(X11_LIBRARIES)