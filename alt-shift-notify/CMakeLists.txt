cmake_minimum_required(VERSION 3.0)
project(alt_shift_notify CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(UDEV REQUIRED libudev)

add_executable(alt_shift_notify
        filesystem.hpp
        global.hpp
        KeyScanner.cpp
        KeyScanner.hpp
        main.cpp
        process-tag.cpp
        process-tag.hpp
        static-sequences.hpp
        SubscriberList.cpp
        SubscriberList.hpp
        UDevContext.cpp
        UDevContext.hpp
        UDevDevice.cpp
        UDevDevice.hpp)

target_include_directories(alt_shift_notify SYSTEM PUBLIC ${UDEV_INCLUDE_DIRS})

target_link_libraries(alt_shift_notify ${CMAKE_THREAD_LIBS_INIT} stdc++fs ${UDEV_LIBRARIES})

install(TARGETS alt_shift_notify RUNTIME DESTINATION bin)