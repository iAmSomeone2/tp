# "dolzel": the game logic. This includes the few actors that live in the DOL (d_a_alink,
# d_a_player, the NPC base classes, ...); the rest are in the actor directory.
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

tp_add_library(dolzel SOURCES ${TP_DOLZEL_SOURCES})

add_subdirectory(actor)
