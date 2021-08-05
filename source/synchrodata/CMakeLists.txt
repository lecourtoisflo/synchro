set(target synchrodata)

add_library(${target} INTERFACE)
target_include_directories(${target} INTERFACE include)

add_library(synchro::${target} ALIAS ${target})
