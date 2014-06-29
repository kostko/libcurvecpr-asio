project(libcurvecpr-asio)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Boost 1.55.0 COMPONENTS system date_time regex REQUIRED)
find_package(Sodium REQUIRED)
find_package(CurveCPR REQUIRED)

include_directories(
  # External includes
  ${Boost_INCLUDE_DIRS}
  ${SODIUM_INCLUDE_DIR}
  ${CURVECPR_INCLUDE_DIR}

  # Internal includes
  ${CMAKE_CURRENT_SOURCE_DIR}/libcurvecpr-asio/include
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(libcurvecpr_asio_external_libraries
  ${Boost_LIBRARIES}
  ${SODIUM_LIBRARY}
  ${CURVECPR_LIBRARY}
)

enable_testing()

add_subdirectory(libcurvecpr-asio)
