# The ~750 actors that were separate relocatable modules (RELs) in the original game. A host
# build links them statically, so they form one library.
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

tp_add_library(actors SOURCES ${TP_ACTORS_SOURCES})
