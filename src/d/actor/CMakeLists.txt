# The ~750 actors that were separate relocatable modules (RELs) in the original game. A host
# build links them statically, so they form one library.
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

# The THP movie player is PowerPC/GameCube specific; src/nightfall/movie replaces it.
list(REMOVE_ITEM TP_ACTORS_SOURCES d_a_movie_player.cpp)

tp_add_library(actors SOURCES ${TP_ACTORS_SOURCES})
