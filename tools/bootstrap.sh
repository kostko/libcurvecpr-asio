#!/bin/bash -e
BOOST_ROOT=$1
LIBCURVECPR_ROOT=$2

if [ -z "${BOOST_ROOT}" ]; then
  echo "ERROR: Missing Boost root parameter."
  exit 1
fi

shift

if [ -z "${LIBCURVECPR_ROOT}" ]; then
  echo "ERROR: Missing libcurvecpr root parameter."
  exit 1
fi

CMAKE_ARGUMENTS=$*

if [ ! -d build ]; then
  echo "ERROR: Missing build directory."
  exit 1
fi

mkdir -p build/{debug,release}

cd build/debug
cmake -DCMAKE_BUILD_TYPE=debug -DBoost_NO_SYSTEM_PATHS=ON -DBOOST_ROOT=${BOOST_ROOT} -DCMAKE_PREFIX_PATH=${LIBCURVECPR_ROOT} ${CMAKE_ARGUMENTS} ../..
cd ../..

cd build/release
cmake -DCMAKE_BUILD_TYPE=release -DBoost_NO_SYSTEM_PATHS=ON -DBOOST_ROOT=${BOOST_ROOT} -DCMAKE_PREFIX_PATH=${LIBCURVECPR_ROOT} ${CMAKE_ARGUMENTS} ../..
cd ../..
