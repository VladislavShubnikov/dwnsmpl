cmake_minimum_required(VERSION 3.1)

project(dsample)
project(test_dsample)

include_directories(
  src/universal
  src/dwnsmpl
  src/win
  src/cspec
  src/test
)

set(dwnsmpl_source_files
  dsample2d.cpp
  dsample2d.h
)

set(universal_source_files
  draw.cpp
  draw.h
  dump.cpp
  dump.h
  image.cpp
  image.h
  ktxtexture.cpp
  ktxtexture.h
  memtrack.cpp
  memtrack.h
  mtypes.cpp
  mtypes.h
  volume.cpp
  volume.h
)

set(win_source_files
  dwnsmp2d_main_win.cpp
)

set(cspec_source_files
  array.c
  cspec.h
  cspec_config.c
  cspec_config.h
  cspec_output.c
  cspec_output.h
  cspec_output_header.c
  cspec_output_header.h
  cspec_output_junit_xml.c
  cspec_output_junit_xml.h
  cspec_output_unit.c
  cspec_output_unit.h
  cspec_output_verbose.c
  cspec_output_verbose.h
  cspec_output_xml.c
  cspec_output_xml.h
  cspec_private.c
  cspec_private.h
  cspec_private_output_junit_xml.h
  cspec_runner.c
)

set(test_source_files
  imgload.cpp
  imgload.h
  main_test.cpp
)

source_group(dwnsmpl   FILES ${dwnsmpl_source_files})
source_group(universal FILES ${universal_source_files})
source_group(win       FILES ${win_source_files})
source_group(cspec     FILES ${cspec_source_files})
source_group(test      FILES ${test_source_files})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W4")
add_executable(dsample WIN32
  src/universal/$<JOIN:${universal_source_files}, src/universal/>
  src/dwnsmpl/$<JOIN:${dwnsmpl_source_files}, src/dwnsmpl/>
  src/win/$<JOIN:${win_source_files}, src/win/>
)
target_link_libraries(dsample gdiplus.lib)

add_executable(test_dsample
  src/universal/$<JOIN:${universal_source_files}, src/universal/>
  src/dwnsmpl/$<JOIN:${dwnsmpl_source_files}, src/dwnsmpl/>
  src/test/$<JOIN:${test_source_files}, src/test/>
  src/cspec/$<JOIN:${cspec_source_files}, src/cspec/>
)
target_link_libraries(test_dsample gdiplus.lib)

