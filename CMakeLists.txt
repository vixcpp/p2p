#  @file CMakeLists.cpp
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira.  All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp — P2P Module (Coreless / Modular)
# ====================================================================
# Required:
#   - vix::net
#
# Optional:
#   - vix::sync  (AUTO|ON|OFF)  (default: AUTO)  (not used yet)
#   - vix::cache (AUTO|ON|OFF)  (default: AUTO)
#   - vix::utils (AUTO|ON|OFF)  (default: AUTO)
#   - JSON backend (AUTO|ON|OFF) (default: AUTO)
#   - OpenSSL (OFF by default)
# ====================================================================

# Standalone vs Umbrella
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.20)
  project(vix_p2p VERSION 1.0.0 LANGUAGES CXX)

  # Global settings (standalone only)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(CMAKE_DISABLE_FIND_PACKAGE_VixOrm ON)

include(GNUInstallDirs)
# ----------------------------------------------------
# Asio (standalone, header-only)
# - Umbrella: must be provided by umbrella as vix::thirdparty_asio
# - Standalone: try to find a vendored copy by walking up directories:
#   - third_party/asio-src/asio/include/asio.hpp   (umbrella layout)
#   - third_party/asio/include/asio.hpp            (legacy layout)
#   - modules/p2p/third_party/asio/include/asio.hpp (module-local)
# ----------------------------------------------------
function(vix_p2p_link_asio onto link_scope)
  # -------------------------
  # Umbrella mode
  # -------------------------
  if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
    if (TARGET vix::thirdparty_asio)
      get_target_property(_asio_inc vix::thirdparty_asio INTERFACE_INCLUDE_DIRECTORIES)
      if (NOT _asio_inc)
        message(FATAL_ERROR "[p2p] Umbrella build: vix::thirdparty_asio has no INTERFACE_INCLUDE_DIRECTORIES.")
      endif()

      target_include_directories(${onto} SYSTEM ${link_scope} ${_asio_inc})
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_ASIO=1)
      return()
    endif()

    message(FATAL_ERROR "[p2p] Umbrella build: vix::thirdparty_asio must be provided by umbrella.")
  endif()

  # -------------------------
  # Standalone mode
  # -------------------------

  function(_vix_find_asio_upwards out_dir start_dir rel_path)
    set(_cur "${start_dir}")
    set(_found "")

    while (TRUE)
      if (EXISTS "${_cur}/${rel_path}/asio.hpp")
        set(_found "${_cur}/${rel_path}")
        break()
      endif()

      get_filename_component(_parent "${_cur}" DIRECTORY)
      if (_parent STREQUAL _cur)
        break()
      endif()
      set(_cur "${_parent}")
    endwhile()

    set(${out_dir} "${_found}" PARENT_SCOPE)
  endfunction()

  set(_asio_local_p2p   "${CMAKE_CURRENT_LIST_DIR}/third_party/asio/include")
  set(_asio_local_http  "${CMAKE_SOURCE_DIR}/third_party/asio/include")

  if (EXISTS "${_asio_local_p2p}/asio.hpp")
    target_include_directories(${onto} SYSTEM ${link_scope} "${_asio_local_p2p}")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_ASIO=1)
    message(STATUS "[p2p] Asio: using module-local Asio at ${_asio_local_p2p}")
    return()
  endif()

  if (EXISTS "${_asio_local_http}/asio.hpp")
    target_include_directories(${onto} SYSTEM ${link_scope} "${_asio_local_http}")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_ASIO=1)
    message(STATUS "[p2p] Asio: using local Asio at ${_asio_local_http}")
    return()
  endif()

  set(_asio_up "")
  _vix_find_asio_upwards(_asio_up "${CMAKE_CURRENT_LIST_DIR}" "third_party/asio-src/asio/include")
  if (_asio_up STREQUAL "")
    _vix_find_asio_upwards(_asio_up "${CMAKE_SOURCE_DIR}" "third_party/asio-src/asio/include")
  endif()

  if (NOT _asio_up STREQUAL "")
    target_include_directories(${onto} SYSTEM ${link_scope} "${_asio_up}")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_ASIO=1)
    message(STATUS "[p2p] Asio: found umbrella Asio at ${_asio_up}")
    return()
  endif()

  set(_asio_up_legacy "")
  _vix_find_asio_upwards(_asio_up_legacy "${CMAKE_CURRENT_LIST_DIR}" "third_party/asio/include")
  if (_asio_up_legacy STREQUAL "")
    _vix_find_asio_upwards(_asio_up_legacy "${CMAKE_SOURCE_DIR}" "third_party/asio/include")
  endif()

  if (NOT _asio_up_legacy STREQUAL "")
    target_include_directories(${onto} SYSTEM ${link_scope} "${_asio_up_legacy}")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_ASIO=1)
    message(STATUS "[p2p] Asio: found legacy Asio at ${_asio_up_legacy}")
    return()
  endif()

  message(FATAL_ERROR
    "[p2p] Asio missing.\n"
    "Searched:\n"
    "  - ${_asio_local_p2p}/asio.hpp\n"
    "  - ${_asio_local_http}/asio.hpp\n"
    "  - <upwards>/third_party/asio-src/asio/include/asio.hpp\n"
    "  - <upwards>/third_party/asio/include/asio.hpp\n"
    "Fix: ensure repo submodule 'third_party/asio-src' is present (umbrella),\n"
    "or vendor Asio inside modules/p2p/third_party/asio/include."
  )
endfunction()

function(vix_p2p_apply_asio_common onto link_scope)
  target_compile_definitions(${onto} ${link_scope} ASIO_STANDALONE=1)

  if (WIN32)
    target_compile_definitions(${onto} ${link_scope} _WIN32_WINNT=0x0A00)
  endif()

  if (UNIX AND NOT APPLE)
    target_link_libraries(${onto} ${link_scope} pthread)
  endif()
endfunction()

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------
# Umbrella build policy:
# When building inside the Vix umbrella, dependencies are managed by the
# umbrella (add_subdirectory order + options). P2P must not FetchContent
# nor add sibling subdirectories.
# --------------------------------------------------------------------
if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_P2P_FETCH_NET   OFF CACHE BOOL "Auto-fetch vix::net if missing" FORCE)
  set(VIX_P2P_FETCH_UTILS OFF CACHE BOOL "Auto-fetch vix::utils if missing" FORCE)
  set(VIX_P2P_FETCH_SYNC  OFF CACHE BOOL "Auto-fetch vix::sync if missing" FORCE)
  set(VIX_P2P_FETCH_CACHE OFF CACHE BOOL "Auto-fetch vix::cache if missing" FORCE)
endif()

# Sources discovery
file(GLOB_RECURSE P2P_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Required: Net
option(VIX_P2P_FETCH_NET "Auto-fetch vix::net if missing" ON)

if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  # Umbrella owns dependency graph: never add_subdirectory/FetchContent here.
  if (NOT TARGET vix::net)
    message(FATAL_ERROR "[p2p] Umbrella build: vix::net must be provided by umbrella before p2p.")
  endif()
else()
  if (NOT TARGET vix::net)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../net/CMakeLists.txt")
      message(STATUS "[p2p] Adding net from umbrella: ../net")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../net" "net")
    elseif (VIX_P2P_FETCH_NET)
      include(FetchContent)
      message(STATUS "[p2p] Fetching vix::net via FetchContent")
      FetchContent_Declare(vix_net
        GIT_REPOSITORY https://github.com/vixcpp/net.git
        GIT_TAG        v0.1.0
      )
      FetchContent_MakeAvailable(vix_net)
    else()
      message(FATAL_ERROR "vix::net not found. Provide it before p2p or enable VIX_P2P_FETCH_NET=ON.")
    endif()
  endif()
endif()

set(VIX_NET_TARGET vix::net)

# Optional: Utils
set(VIX_P2P_WITH_UTILS "AUTO" CACHE STRING "Link utils module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_UTILS PROPERTY STRINGS AUTO ON OFF)
option(VIX_P2P_FETCH_UTILS "Auto-fetch vix::utils if missing (when required)" ON)

function(vix_p2p_try_link_utils onto link_scope)
  if (VIX_P2P_WITH_UTILS STREQUAL "OFF")
    message(STATUS "[p2p] Utils disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_UTILS=1)
    return()
  endif()

  # Umbrella mode: deps are owned by umbrella (no add_subdirectory / no FetchContent here)
  if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
    if (TARGET vix::utils)
      message(STATUS "[p2p] Utils backend: vix::utils (umbrella)")
      target_link_libraries(${onto} ${link_scope} vix::utils)
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_UTILS=1)
    else()
      if (VIX_P2P_WITH_UTILS STREQUAL "ON")
        message(FATAL_ERROR "Utils required but vix::utils not found (umbrella build).")
      else()
        message(STATUS "[p2p] Utils not detected; building without utils. (umbrella)")
        target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_UTILS=1)
      endif()
    endif()
    return()
  endif()

  # Standalone mode: may add sibling or FetchContent
  if (NOT TARGET vix::utils)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../utils/CMakeLists.txt")
      message(STATUS "[p2p] Adding utils from umbrella: ../utils")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../utils" "utils")
    elseif (VIX_P2P_FETCH_UTILS)
      include(FetchContent)
      message(STATUS "[p2p] Fetching vix::utils via FetchContent")
      FetchContent_Declare(vix_utils
        GIT_REPOSITORY https://github.com/vixcpp/utils.git
        GIT_TAG        dev
      )
      FetchContent_MakeAvailable(vix_utils)
    endif()
  endif()

  if (TARGET vix::utils)
    message(STATUS "[p2p] Utils backend: vix::utils")
    target_link_libraries(${onto} ${link_scope} vix::utils)
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_UTILS=1)
  else()
    if (VIX_P2P_WITH_UTILS STREQUAL "ON")
      message(FATAL_ERROR "Utils required but vix::utils not found.")
    else()
      message(STATUS "[p2p] Utils not detected; building without utils.")
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_UTILS=1)
    endif()
  endif()
endfunction()


# Optional: Sync
set(VIX_P2P_WITH_SYNC "AUTO" CACHE STRING "Link sync module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_SYNC PROPERTY STRINGS AUTO ON OFF)
option(VIX_P2P_FETCH_SYNC "Auto-fetch vix::sync if missing (when required)" ON)

function(vix_p2p_try_link_sync onto link_scope)
  if (VIX_P2P_WITH_SYNC STREQUAL "OFF")
    message(STATUS "[p2p] Sync disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_SYNC=1)
    return()
  endif()

  # Umbrella mode: deps are owned by umbrella (no add_subdirectory / no FetchContent here)
  if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
    if (TARGET vix::sync)
      message(STATUS "[p2p] Sync backend: vix::sync (umbrella)")
      target_link_libraries(${onto} ${link_scope} vix::sync)
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_SYNC=1)
    else()
      if (VIX_P2P_WITH_SYNC STREQUAL "ON")
        message(FATAL_ERROR "Sync required but vix::sync not found (umbrella build).")
      else()
        message(STATUS "[p2p] Sync not detected; building without sync. (umbrella)")
        target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_SYNC=1)
      endif()
    endif()
    return()
  endif()

  # Standalone mode: may add sibling or FetchContent
  if (NOT TARGET vix::sync)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../sync/CMakeLists.txt")
      message(STATUS "[p2p] Adding sync from umbrella: ../sync")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../sync" "sync")
    elseif (VIX_P2P_FETCH_SYNC)
      include(FetchContent)
      message(STATUS "[p2p] Fetching vix::sync via FetchContent")
      FetchContent_Declare(vix_sync
        GIT_REPOSITORY https://github.com/vixcpp/sync.git
        GIT_TAG        v0.1.0
      )
      FetchContent_MakeAvailable(vix_sync)
    endif()
  endif()

  if (TARGET vix::sync)
    message(STATUS "[p2p] Sync backend: vix::sync")
    target_link_libraries(${onto} ${link_scope} vix::sync)
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_SYNC=1)
  else()
    if (VIX_P2P_WITH_SYNC STREQUAL "ON")
      message(FATAL_ERROR "Sync required but vix::sync not found.")
    else()
      message(STATUS "[p2p] Sync not detected; building without sync.")
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_SYNC=1)
    endif()
  endif()
endfunction()


# Optional: Cache
set(VIX_P2P_WITH_CACHE "AUTO" CACHE STRING "Link cache module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_CACHE PROPERTY STRINGS AUTO ON OFF)
option(VIX_P2P_FETCH_CACHE "Auto-fetch vix::cache if missing (when required)" ON)

function(vix_p2p_try_link_cache onto link_scope)
  if (VIX_P2P_WITH_CACHE STREQUAL "OFF")
    message(STATUS "[p2p] Cache disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_CACHE=1)
    return()
  endif()

  # Umbrella mode: deps are owned by umbrella (no add_subdirectory / no FetchContent here)
  if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
    if (TARGET vix::cache)
      message(STATUS "[p2p] Cache backend: vix::cache (umbrella)")
      target_link_libraries(${onto} ${link_scope} vix::cache)
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_CACHE=1)
    else()
      if (VIX_P2P_WITH_CACHE STREQUAL "ON")
        message(FATAL_ERROR "Cache required but vix::cache not found (umbrella build).")
      else()
        message(STATUS "[p2p] Cache not detected; building without cache. (umbrella)")
        target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_CACHE=1)
      endif()
    endif()
    return()
  endif()

  # Standalone mode: may add sibling or FetchContent
  if (NOT TARGET vix::cache)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../cache/CMakeLists.txt")
      message(STATUS "[p2p] Adding cache from umbrella: ../cache")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../cache" "cache")
    elseif (VIX_P2P_FETCH_CACHE)
      include(FetchContent)
      message(STATUS "[p2p] Fetching vix::cache via FetchContent")
      FetchContent_Declare(vix_cache
        GIT_REPOSITORY https://github.com/vixcpp/cache.git
        GIT_TAG        v0.1.0
      )
      FetchContent_MakeAvailable(vix_cache)
    endif()
  endif()

  if (TARGET vix::cache)
    message(STATUS "[p2p] Cache backend: vix::cache")
    target_link_libraries(${onto} ${link_scope} vix::cache)
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_WITH_CACHE=1)
  else()
    if (VIX_P2P_WITH_CACHE STREQUAL "ON")
      message(FATAL_ERROR "Cache required but vix::cache not found.")
    else()
      message(STATUS "[p2p] Cache not detected; building without cache.")
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_CACHE=1)
    endif()
  endif()
endfunction()

# Optional: JSON backend
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
      message(FATAL_ERROR "JSON required but not found (expected: vix::json or vix_json).")
    else()
      message(STATUS "[p2p] No JSON backend detected; building without JSON.")
      target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_JSON=1)
    endif()
  endif()
endfunction()

#  Optional SSL
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
    message(FATAL_ERROR "VIX_P2P_WITH_SSL=ON but OpenSSL was not found.")
  endif()
endfunction()

# STATIC
if (P2P_SOURCES)
  message(STATUS "[p2p] Building STATIC library with detected sources.")

  add_library(vix_p2p STATIC ${P2P_SOURCES})

if (NOT MSVC)
  target_compile_options(vix_p2p PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(vix_p2p PRIVATE
    -fno-omit-frame-pointer
    -fsanitize=address,undefined
  )
  target_link_options(vix_p2p PRIVATE
    -fsanitize=address,undefined
  )
endif()

  add_library(vix::p2p ALIAS vix_p2p)
  target_compile_features(vix_p2p PUBLIC cxx_std_20)

target_include_directories(vix_p2p
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

  # Required
  target_link_libraries(vix_p2p PUBLIC ${VIX_NET_TARGET})
  vix_p2p_link_asio(vix_p2p PRIVATE)
  vix_p2p_apply_asio_common(vix_p2p PRIVATE)

  # Optional deps
  vix_p2p_try_link_utils(vix_p2p PUBLIC)
  vix_p2p_try_link_sync(vix_p2p PUBLIC)
  vix_p2p_try_link_cache(vix_p2p PUBLIC)
  vix_p2p_try_link_json(vix_p2p PUBLIC)
  vix_p2p_try_link_ssl(vix_p2p PUBLIC)

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

# HEADER-ONLY
else()
  message(STATUS "[p2p] Building HEADER-ONLY library (no sources).")

  add_library(vix_p2p INTERFACE)
  add_library(vix::p2p ALIAS vix_p2p)

  target_compile_features(vix_p2p INTERFACE cxx_std_20)

 target_include_directories(vix_p2p INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

  target_link_libraries(vix_p2p INTERFACE ${VIX_NET_TARGET})
  vix_p2p_link_asio(vix_p2p INTERFACE)
  vix_p2p_apply_asio_common(vix_p2p INTERFACE)

  vix_p2p_try_link_utils(vix_p2p INTERFACE)
  vix_p2p_try_link_sync(vix_p2p INTERFACE)
  vix_p2p_try_link_cache(vix_p2p INTERFACE)
  vix_p2p_try_link_json(vix_p2p INTERFACE)
  vix_p2p_try_link_ssl(vix_p2p INTERFACE)

  install(TARGETS vix_p2p
    EXPORT VixTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
endif()

# Tests
option(VIX_P2P_BUILD_TESTS "Build p2p tests" OFF)

if (VIX_P2P_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# Summary
message(STATUS "------------------------------------------------------")
message(STATUS "vix::p2p configured (${PROJECT_VERSION})")
if (P2P_SOURCES)
  message(STATUS "Mode: STATIC / sources found")
else()
  message(STATUS "Mode: HEADER-ONLY / no sources")
endif()
message(STATUS "Utils policy:      ${VIX_P2P_WITH_UTILS}")
message(STATUS "Sync policy:       ${VIX_P2P_WITH_SYNC}")
message(STATUS "Cache policy:      ${VIX_P2P_WITH_CACHE}")
message(STATUS "JSON policy:       ${VIX_P2P_WITH_JSON}")
message(STATUS "SSL enabled:       ${VIX_P2P_WITH_SSL}")
message(STATUS "Net target:        ${VIX_NET_TARGET}")
message(STATUS "Include dir:       ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Sanitizers enabled:${VIX_ENABLE_SANITIZERS}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
