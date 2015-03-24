package(default_visibility = ["//visibility:public"])

cc_library(
    name = "weeder",
    srcs = [
        "assignment_visitor.cpp",
        "call_visitor.cpp",
        "int_range_visitor.cpp",
        "modifier_visitor.cpp",
        "structure_visitor.cpp",
        "type_visitor.cpp",
        "weeder.cpp",
    ],
    hdrs = [
        "assignment_visitor.h",
        "call_visitor.h",
        "int_range_visitor.h",
        "modifier_visitor.h",
        "structure_visitor.h",
        "type_visitor.h",
        "weeder.h",
    ],
    deps = [
        "//ast",
        "//base",
        "//lexer",
        "//parser",
    ],
)
