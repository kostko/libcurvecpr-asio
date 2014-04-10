project(libcurvecpr-asio)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS OFF)

find_package(Boost 1.55.0 COMPONENTS system date_time REQUIRED)
find_package(Sodium REQUIRED)
find_package(CurveCPR REQUIRED)
find_package(Botan REQUIRED)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fvisibility=hidden -DBOOST_ALL_DYN_LINK -std=c++0x")
set(CMAKE_C_FLAGS_RELEASE "-O3 -fvisibility=hidden -DBOOST_ALL_DYN_LINK")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fvisibility=hidden -DBOOST_ALL_DYN_LINK -std=c++0x")
set(CMAKE_C_FLAGS_DEBUG "-g3 -fvisibility=hidden -DBOOST_ALL_DYN_LINK")

include_directories(
  # External includes
  ${Boost_INCLUDE_DIRS}
  ${SODIUM_INCLUDE_DIR}
  ${CURVECPR_INCLUDE_DIR}
  ${BOTAN_INCLUDE_DIR}

  # Internal includes
  ${CMAKE_CURRENT_SOURCE_DIR}/libcurvecpr-asio/include
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(libcurvecpr_asio_external_libraries
  ${Boost_LIBRARIES}
  ${SODIUM_LIBRARY}
  ${CURVECPR_LIBRARY}
  ${BOTAN_LIBRARIES}
)

enable_testing()

add_subdirectory(libcurvecpr-asio)
