# - Find fftw
# Find the native fftw includes and libraries
#
#  FFTW_INCLUDE_DIRS - where to find fftw.h, etc.
#  FFTW_LIBRARIES    - List of libraries when using fftw.
#  FFTW_FOUND        - True if fftw found.

if(FFTW_INCLUDE_DIR)
    # Already in cache, be silent
    set(FFTW_FIND_QUIETLY TRUE)
endif(FFTW_INCLUDE_DIR)

find_package (PkgConfig QUIET)
pkg_check_modules(PC_FFTW QUIET fftw3f)

find_path(FFTW_INCLUDE_DIR fftw3.h HINTS ${PC_FFTW_INCLUDEDIR} ${PC_FFTW_INCLUDE_DIRS} ${FFTW_ROOT} PATH_SUFFIXES include)
find_library(FFTW_LIBRARY NAMES fftw3f HINTS ${PC_FFTW_LIBDIR} ${PC_FFTW_LIBRARY_DIRS} ${FFTW_ROOT} PATH_SUFFIXES lib)
# Handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW DEFAULT_MSG FFTW_INCLUDE_DIR FFTW_LIBRARY)

if (FFTW_FOUND)
	set (FFTW_LIBRARIES ${FFTW_LIBRARY})
	set (FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR})
endif (FFTW_FOUND)

mark_as_advanced(FFTW_INCLUDE_DIR FFTW_LIBRARY)
