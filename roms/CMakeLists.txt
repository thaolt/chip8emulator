cmake_minimum_required(VERSION 2.8)

project(roms)

file(GLOB_RECURSE ROMS "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(FILTER ROMS EXCLUDE REGEX "CMakeLists\\.txt")

add_custom_target(mk_roms_dir)

add_custom_target(roms_copy ALL)

foreach(ROM ${ROMS})
  add_custom_command(TARGET roms_copy POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                         copy ${ROM} ${CMAKE_BINARY_DIR}/dist/roms/)
endforeach()

add_dependencies(roms_copy mk_roms_dir)

add_custom_command(TARGET mk_roms_dir PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E
                   make_directory ${CMAKE_BINARY_DIR}/dist/roms/)
