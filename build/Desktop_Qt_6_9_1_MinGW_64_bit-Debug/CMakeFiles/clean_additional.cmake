# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\MemoryWriter_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\MemoryWriter_autogen.dir\\ParseCache.txt"
  "MemoryWriter_autogen"
  )
endif()
