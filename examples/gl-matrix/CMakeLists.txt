set(SRC_FILES main.cpp)


add_executable(gl-matrix ${SRC_FILES})

target_link_libraries(gl-matrix fort-charis::libfort-gl)
