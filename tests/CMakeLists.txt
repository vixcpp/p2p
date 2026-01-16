cmake_minimum_required(VERSION 3.20)

add_executable(p2p_demo
  manual/main.cpp
)

target_link_libraries(p2p_demo
  PRIVATE
    vix::p2p
)

target_compile_definitions(p2p_demo
  PRIVATE
    ASIO_STANDALONE=1
)

find_package(Threads REQUIRED)
target_link_libraries(p2p_demo PRIVATE Threads::Threads)
add_test(NAME vix_p2p_manual_demo COMMAND p2p_demo --help)
