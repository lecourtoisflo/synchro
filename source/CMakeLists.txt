add_subdirectory(synchrodata)

# Always list the source files explicitly, including headers so that they are listed in the IDE
# If you need to use files based on a variable value, use target_sources
add_executable(Synchro main.cpp)

target_link_libraries(Synchro
synchro::synchrodata
)
