package(default_visibility = ["//visibility:public"])

cc_library(
    name = "types",
    srcs = [
        "constant_folding_visitor.cpp",
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
        "typeset_impl.cpp",
    ],
    hdrs = [
        "constant_folding_visitor.h",
        "dataflow_visitor.h",
        "decl_resolver.h",
        "symbol_table.h",
        "type_info_map.h",
        "typechecker.h",
        "types.h",
        "types_internal.h",
        "typeset.h",
        "typeset_impl.h",
    ],
    deps = [
        "//ast",
        "//base",
        "//lexer",
    ],
)

cc_test(
    name = "types_test",
    srcs = [
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
        "//parser",
        "//third_party/gtest:main",
        ":types",
    ],
    data = [
        "//third_party/cs444/stdlib:3",
    ],
    size = "small",
)
