package(default_visibility = ["//visibility:public"])

exports_files(["std.h"]);

cc_binary(
    name = "joosc",
    srcs = [
        "joosc_main.cpp",
    ],
    deps = [
        ":joosc_lib",
    ],
)

cc_library(
    name = "joosc_lib",
    srcs = [
        "joosc.cpp",
    ],
    hdrs = [
        "joosc.h",
    ],
    deps = [
        "//ast",
        "//backend/common",
        "//backend/i386",
        "//base",
        "//ir",
        "//lexer",
        "//parser",
        "//runtime",
        "//types",
        "//weeder",
    ],
)

test_suite(
    name = "all_tests",
    tests = [
        "//base:base_test",
        "//lexer:lexer_test",
        "//marmoset:a1",
        "//marmoset:a2",
        "//marmoset:a3",
        "//marmoset:a4",
        "//parser:parser_test",
        "//types:types_test",
        "//weeder:weeder_test",
    ],
)
