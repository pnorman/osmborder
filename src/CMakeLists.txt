#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSMBorder
#
#-----------------------------------------------------------------------------

add_executable(osmborder_filter osmborder_filter.cpp)
target_link_libraries(osmborder_filter ${OSMIUM_IO_LIBRARIES} ${GETOPT_LIBRARY})
install(TARGETS osmborder_filter DESTINATION bin)
