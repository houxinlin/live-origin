#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "maxminddb::maxminddb" for configuration ""
set_property(TARGET maxminddb::maxminddb APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(maxminddb::maxminddb PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libmaxminddb.a"
  )

list(APPEND _cmake_import_check_targets maxminddb::maxminddb )
list(APPEND _cmake_import_check_files_for_maxminddb::maxminddb "${_IMPORT_PREFIX}/lib/libmaxminddb.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
