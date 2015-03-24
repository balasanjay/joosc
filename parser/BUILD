package(default_visibility = ["//visibility:public"])

cc_library(
    name = "parser",
    srcs = [
        "parser.cpp",
    ],
    hdrs = [
        "parser.h",
        "parser_internal.h",
    ],
    deps = [
        "//ast",
        "//base",
        "//lexer",
    ],
)

cc_test(
    name = "parser_test",
    srcs = [
        "parser_test.cpp",
    ],
    deps = [
        "//third_party/gtest:main",
        ":parser",
    ],
    size = "small",
)
