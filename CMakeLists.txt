cmake_minimum_required(VERSION 2.6.2)

project("rssproxy")

add_definitions(-std=c++0x)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system program_options thread)

find_package(Threads)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

add_executable(${PROJECT_NAME}
                    main.cpp
                    server.cpp
                    serverconfig.cpp
                    client.cpp
                    uri.cpp
                    rfc882/rfc882.cpp)

include_directories(.
                    /usr/include)

target_link_libraries(${PROJECT_NAME}
                            ${Boost_SYSTEM_LIBRARY}
                            ${Boost_PROGRAM_OPTIONS_LIBRARY}
                            ${Boost_THREAD_LIBRARY}
                            ${CMAKE_THREAD_LIBS_INIT}
                            pugixml)
