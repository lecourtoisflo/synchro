## Googletest dependency
find_package(GTest REQUIRED)
if(NOT TARGET GTest::gtest AND TARGET GTest::GTest)
  add_library(GTest::gtest UNKNOWN IMPORTED)
  if(EXISTS "${GTEST_LIBRARY}")
    set_target_properties(GTest::gtest PROPERTIES
            IMPORTED_LOCATION "${GTEST_LIBRARY}")
  endif()
endif()
if(NOT TARGET GTest::gtest_main AND TARGET GTest::Main)
  add_library(GTest::gtest_main UNKNOWN IMPORTED)
  if(EXISTS "${GTEST_MAIN_LIBRARY}")
    set_target_properties(GTest::gtest_main PROPERTIES
            IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}")
  endif()
endif()
find_package(GMock REQUIRED)
add_library(SynchroTest INTERFACE)
target_link_libraries(SynchroTest INTERFACE
  GTest::gmock_main
  GTest::gtest_main)

add_subdirectory(synchrodata)