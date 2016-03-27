package(default_visibility = ["//visibility:public"])

cc_library(
    name = "types",
    srcs = [
        "constant_folding.cpp",
        "dataflow_visitor.cpp",
        "decl_resolver.cpp",
        "symbol_table.cpp",
        "type_info_map.cpp",
        "typechecker.cpp",
        "typechecker_errors.cpp",
        "typechecker_utils.cpp",
        "types.cpp",
        "types_internal.cpp",
        "typeset.cpp",
    ],
    hdrs = [
        "constant_folding.h",
        "dataflow_visitor.h",
        "decl_resolver.h",
        "symbol_table.h",
        "type_info_map.h",
        "typechecker.h",
        "types.h",
        "types_internal.h",
        "typeset.h",
    ],
    deps = [
        "//ast",
        "//base",
        "//external:googletest",
        "//lexer",
    ],
)

cc_test(
    name = "types_test",
    srcs = [
        "constant_folding_test.cpp",
        "field_table_test.cpp",
        "method_table_test.cpp",
        "symbol_table_test.cpp",
        "type_info_map_test.cpp",
        "typechecker_hierarchy_test.cpp",
        "typechecker_test.cpp",
        "types_test.cpp",
        "types_test.h",
        "typeset_test.cpp",
    ],
    deps = [
        "//:joosc_lib",
        "//external:googletest_main",
        "//parser",
        ":types",
    ],
    data = [
        "//third_party/cs444/stdlib:3",
    ],
    size = "small",
)
