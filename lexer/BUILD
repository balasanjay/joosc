package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lexer",
    srcs = [
        "lexer.cpp",
    ],
    hdrs = [
        "lexer.h",
        "lexer_error.h",
    ],
    deps = [
        "//base",
    ],
)

cc_test(
    name = "lexer_test",
    srcs = [
        "lexer_test.cpp",
    ],
    deps = [
        "//external:googletest_main",
        ":lexer",
    ],
    size = "small",
)
