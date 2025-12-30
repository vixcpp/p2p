# ====================================================================
# Vix.cpp — P2P Module
# ====================================================================
# Purpose:
#   Build configuration for the Vix peer-to-peer & edge networking module:
#   - Node discovery and peer management
#   - Transport layer (TCP first; QUIC later)
#   - Message framing + protocol envelopes
#   - Edge routing / relay (store-and-forward)
#   - Transport backbone for Vix offline-first sync (WAL / outbox) via EdgeSync
#
# Public Targets:
#   - vix_p2p     : The actual library target (STATIC or INTERFACE)
#   - vix::p2p    : Canonical namespaced alias
#
# Public Dependencies:
#   - vix::core   (net primitives, common types, logging helpers)
#   - optional JSON backend (vix::json / vix_json) for debugging/interop (optional)
#   - optional OpenSSL (future): handshake / TLS, if enabled
#
# Options:
#   - VIX_P2P_WITH_JSON   : AUTO|ON|OFF (default: AUTO)
#   - VIX_P2P_FETCH_CORE  : Auto-fetch vix::core if missing (default ON)
#   - VIX_P2P_WITH_SSL    : ON|OFF (default OFF)  (future-ready)
#
# Installation/Export:
#   - Contributes to the umbrella export-set `VixTargets`
#   - Headers installed under <prefix>/include/vix/p2p/...
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_p2p VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_DISABLE_FIND_PACKAGE_VixOrm ON)

include(GNUInstallDirs)

# ------------------------ Global settings ----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------ Sources discovery --------------------------
file(GLOB_RECURSE P2P_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# ------------------------ Core dependency ----------------------------
option(VIX_P2P_FETCH_CORE "Auto-fetch vix::core if missing" ON)

if (NOT TARGET vix::core)
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../core/CMakeLists.txt")
    message(STATUS "[p2p] Adding core from umbrella: ../core")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../core" "core")
  elseif (VIX_P2P_FETCH_CORE)
    include(FetchContent)
    message(STATUS "[p2p] Fetching vix::core via FetchContent")
    FetchContent_Declare(vix_core
      GIT_REPOSITORY https://github.com/vixcpp/core.git
      GIT_TAG        v1.5.2
    )
    FetchContent_MakeAvailable(vix_core)
  else()
    message(FATAL_ERROR
      "vix::core not found. Provide vix::core before p2p "
      "or enable VIX_P2P_FETCH_CORE=ON.")
  endif()
endif()

set(VIX_CORE_TARGET vix::core)

# ------------------------ JSON linkage policy ------------------------
set(VIX_P2P_WITH_JSON "AUTO" CACHE STRING "Link JSON backend (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_JSON PROPERTY STRINGS AUTO ON OFF)

function(vix_p2p_try_link_json onto link_scope)
  if (VIX_P2P_WITH_JSON STREQUAL "OFF")
    message(STATUS "[p2p] JSON disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_JSON=1)
    return()
  endif()

  set(_json_candidates vix::json vix_json)
  set(_json_found "")
  foreach(t IN LISTS _json_candidates)
    if (TARGET ${t})
      set(_json_found ${t})
      break()
    endif()
  endforeach()

  if (_json_found)
    message(STATUS "[p2p] JSON backend: ${_json_found}")
    target_link_libraries(${onto} ${link_scope} ${_json_found})
  else()
    if (VIX_P2P_WITH_JSON STREQUAL "ON")
      message(FATAL_ERROR
        "JSON required but not found (expected: vix::json or vix_json).")
    else()
      message(STATUS "[p2p] No JSON backend detected; building without JSON.")
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_JSON=1)
    endif()
  endif()
endfunction()

# ------------------------ Optional SSL (future-ready) ----------------
option(VIX_P2P_WITH_SSL "Enable OpenSSL integration for P2P" OFF)

function(vix_p2p_try_link_ssl onto link_scope)
  if (NOT VIX_P2P_WITH_SSL)
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_SSL=1)
    message(STATUS "[p2p] SSL disabled (OFF)")
    return()
  endif()

  find_package(OpenSSL QUIET)
  if (OpenSSL_FOUND)
    message(STATUS "[p2p] OpenSSL found: ${OpenSSL_VERSION}")
    target_link_libraries(${onto} ${link_scope} OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_SSL=1)
  else()
    message(FATAL_ERROR
      "VIX_P2P_WITH_SSL=ON but OpenSSL was not found. "
      "Install OpenSSL dev package or set VIX_P2P_WITH_SSL=OFF.")
  endif()
endfunction()

# ============================== STATIC ===============================
if (P2P_SOURCES)
  message(STATUS "[p2p] Building STATIC library with detected sources.")

  add_library(vix_p2p STATIC ${P2P_SOURCES})
  if (TARGET vix_warnings)
    target_link_libraries(vix_p2p PUBLIC vix_warnings)
  endif()

  add_library(vix::p2p ALIAS vix_p2p)
  target_compile_features(vix_p2p PUBLIC cxx_std_20)

  target_include_directories(vix_p2p
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_p2p
    PUBLIC
      ${VIX_CORE_TARGET}
  )

  vix_p2p_try_link_json(vix_p2p PUBLIC)
  vix_p2p_try_link_ssl(vix_p2p PUBLIC)

  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_p2p PUBLIC vix_sanitizers)
  endif()

  set_target_properties(vix_p2p PROPERTIES
    OUTPUT_NAME vix_p2p
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME p2p
  )

  install(TARGETS vix_p2p
    EXPORT VixTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# ============================ HEADER-ONLY ============================
else()
  message(STATUS "[p2p] Building HEADER-ONLY library (no sources).")

  add_library(vix_p2p INTERFACE)

  if (TARGET vix_warnings)
    target_link_libraries(vix_p2p INTERFACE vix_warnings)
  endif()

  add_library(vix::p2p ALIAS vix_p2p)
  target_compile_features(vix_p2p INTERFACE cxx_std_20)

  target_include_directories(vix_p2p INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_p2p INTERFACE
    ${VIX_CORE_TARGET}
  )

  vix_p2p_try_link_json(vix_p2p INTERFACE)
  vix_p2p_try_link_ssl(vix_p2p INTERFACE)

  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_p2p INTERFACE vix_sanitizers)
  endif()

  install(TARGETS vix_p2p
    EXPORT VixTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
endif()

# ------------------------ Tests (optional) ---------------------------
option(VIX_P2P_BUILD_TESTS "Build p2p tests" OFF)

if (VIX_P2P_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# ----------------------------- Summary -------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "vix::p2p configured (${PROJECT_VERSION})")
if (P2P_SOURCES)
  message(STATUS "Mode: STATIC / sources found")
else()
  message(STATUS "Mode: HEADER-ONLY / no sources")
endif()
message(STATUS "JSON policy:        ${VIX_P2P_WITH_JSON}")
message(STATUS "SSL enabled:        ${VIX_P2P_WITH_SSL}")
message(STATUS "Core target:        ${VIX_CORE_TARGET}")
message(STATUS "Include dir:        ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Sanitizers enabled: ${VIX_ENABLE_SANITIZERS}")
message(STATUS "Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
