
add_library(migraphx_fpga
    target.cpp
    lowering.cpp
    partitioning.cpp
    vitis_ai_adapter.cpp
)

set_target_properties(migraphx_fpga PROPERTIES EXPORT_NAME fpga)
rocm_set_soversion(migraphx_fpga ${MIGRAPHX_SO_VERSION})

rocm_clang_tidy_check(migraphx_fpga)
target_link_libraries(migraphx_fpga migraphx)

rocm_install_targets(
    TARGETS migraphx_fpga
    INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
