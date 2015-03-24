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
