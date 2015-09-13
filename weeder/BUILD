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

cc_test(
    name = "weeder_test",
    srcs = [
        "assignment_visitor_test.cpp",
        "call_visitor_test.cpp",
        "int_range_visitor_test.cpp",
        "modifier_visitor_test.cpp",
        "structure_visitor_test.cpp",
        "type_visitor_test.cpp",
        "weeder_test.h",
    ],
    deps = [
        "//external:googletest_main",
        ":weeder",
    ],
    size = "small",
)
