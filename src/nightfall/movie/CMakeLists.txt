add_library(nf_movie STATIC movie_player.cpp)
target_link_libraries(nf_movie PUBLIC tp::config tp::JSystem_headers tp::dolphin PRIVATE tp::warnings)
set_target_properties(nf_movie PROPERTIES CXX_EXTENSIONS OFF)

add_library(nf::movie ALIAS nf_movie)
set_property(GLOBAL APPEND PROPERTY TP_LIBRARIES nf_movie)