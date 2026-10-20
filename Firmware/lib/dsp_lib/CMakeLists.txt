idf_component_register(
    SRCS
        "CommonTables/xtensa_common_tables.c"
        "FastMathFunctions/xtensa_sin_f32.c"
        "FastMathFunctions/xtensa_cos_f32.c"
        "MatrixFunctions/xtensa_mat_trans_f32.c"
        "MatrixFunctions/xtensa_mat_mult_f32.c"
        "StatisticsFunctions/xtensa_mean_f32.c"
    INCLUDE_DIRS "include"
)

target_compile_options(${COMPONENT_LIB} PUBLIC "-fno-strict-aliasing")
