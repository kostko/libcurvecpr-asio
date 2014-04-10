
# - Find CurveCPR
# Find the native libcurvecpr includes and library.
# Once done this will define
#
#  CURVECPR_INCLUDE_DIR    - where to find libcurvecpr header files, etc.
#  CURVECPR_LIBRARY        - List of libraries when using libcurvecpr.
#  CURVECPR_FOUND          - True if libcurvecpr found.
#

FIND_LIBRARY(CURVECPR_LIBRARY NAMES libcurvecpr.so HINTS ${CURVECPR_LIB_DIR})
FIND_PATH(CURVECPR_INCLUDE_DIR curvecpr.h)

# handle the QUIETLY and REQUIRED arguments and set CURVECPR_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CurveCPR REQUIRED_VARS CURVECPR_LIBRARY CURVECPR_INCLUDE_DIR)

MARK_AS_ADVANCED(CURVECPR_LIBRARY CURVECPR_INCLUDE_DIR)
