cmake_minimum_required(VERSION 3.20)

# On a déjà enable_testing() dans le CMake parent.
# Ici on build juste nos exécutables de test / manual.

add_executable(p2p_demo
  manual/main.cpp
)

target_link_libraries(p2p_demo
  PRIVATE
    vix::p2p
)

# Asio standalone
target_compile_definitions(p2p_demo
  PRIVATE
    ASIO_STANDALONE=1
)

# pthread (Linux)
find_package(Threads REQUIRED)
target_link_libraries(p2p_demo PRIVATE Threads::Threads)

# Optionnel: rendre le binaire visible dans ctest
add_test(NAME vix_p2p_manual_demo COMMAND p2p_demo --help)
