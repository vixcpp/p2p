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

# ------------------------ Required: Net ------------------------------
option(VIX_P2P_FETCH_NET "Auto-fetch vix::net if missing" ON)

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

set(VIX_NET_TARGET vix::net)

# ------------------------ Optional: Utils ----------------------------
set(VIX_P2P_WITH_UTILS "AUTO" CACHE STRING "Link utils module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_UTILS PROPERTY STRINGS AUTO ON OFF)
option(VIX_P2P_FETCH_UTILS "Auto-fetch vix::utils if missing (when required)" ON)

function(vix_p2p_try_link_utils onto link_scope)
  if (VIX_P2P_WITH_UTILS STREQUAL "OFF")
    message(STATUS "[p2p] Utils disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_UTILS=1)
    return()
  endif()

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

# ------------------------ Optional: Sync -----------------------------
set(VIX_P2P_WITH_SYNC "AUTO" CACHE STRING "Link sync module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_SYNC PROPERTY STRINGS AUTO ON OFF)
option(VIX_P2P_FETCH_SYNC "Auto-fetch vix::sync if missing (when required)" ON)

function(vix_p2p_try_link_sync onto link_scope)
  if (VIX_P2P_WITH_SYNC STREQUAL "OFF")
    message(STATUS "[p2p] Sync disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_SYNC=1)
    return()
  endif()

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

# ------------------------ Optional: Cache ----------------------------
set(VIX_P2P_WITH_CACHE "AUTO" CACHE STRING "Link cache module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_WITH_CACHE PROPERTY STRINGS AUTO ON OFF)
option(VIX_P2P_FETCH_CACHE "Auto-fetch vix::cache if missing (when required)" ON)

function(vix_p2p_try_link_cache onto link_scope)
  if (VIX_P2P_WITH_CACHE STREQUAL "OFF")
    message(STATUS "[p2p] Cache disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_P2P_NO_CACHE=1)
    return()
  endif()

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

# ------------------------ Optional: JSON backend ----------------------
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
    message(FATAL_ERROR "VIX_P2P_WITH_SSL=ON but OpenSSL was not found.")
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

  # Required
  target_link_libraries(vix_p2p PUBLIC ${VIX_NET_TARGET})

  # Optional deps
  vix_p2p_try_link_utils(vix_p2p PUBLIC)
  vix_p2p_try_link_sync(vix_p2p PUBLIC)
  vix_p2p_try_link_cache(vix_p2p PUBLIC)
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
  add_library(vix::p2p ALIAS vix_p2p)

  if (TARGET vix_warnings)
    target_link_libraries(vix_p2p INTERFACE vix_warnings)
  endif()

  target_compile_features(vix_p2p INTERFACE cxx_std_20)

  target_include_directories(vix_p2p INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_p2p INTERFACE ${VIX_NET_TARGET})

  vix_p2p_try_link_utils(vix_p2p INTERFACE)
  vix_p2p_try_link_sync(vix_p2p INTERFACE)
  vix_p2p_try_link_cache(vix_p2p INTERFACE)
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
