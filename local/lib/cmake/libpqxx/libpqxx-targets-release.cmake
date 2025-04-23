#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libpqxx::pqxx" for configuration "Release"
set_property(TARGET libpqxx::pqxx APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libpqxx::pqxx PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpqxx-7.10.so"
  IMPORTED_SONAME_RELEASE "libpqxx-7.10.so"
  )

list(APPEND _cmake_import_check_targets libpqxx::pqxx )
list(APPEND _cmake_import_check_files_for_libpqxx::pqxx "${_IMPORT_PREFIX}/lib/libpqxx-7.10.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
