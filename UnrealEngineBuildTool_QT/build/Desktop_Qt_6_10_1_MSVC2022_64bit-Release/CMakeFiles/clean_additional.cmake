# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\UnrealEngineBuildTool_QT_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\UnrealEngineBuildTool_QT_autogen.dir\\ParseCache.txt"
  "UnrealEngineBuildTool_QT_autogen"
  )
endif()
