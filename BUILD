package(default_visibility = ["//visibility:public"])

exports_files([
    "asm.sh",
    "std.h",
]);

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

cc_library(
    name = "std",
    hdrs = ["std.h"],
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
        "//marmoset:a5",
        "//parser:parser_test",
        "//runtime/joostests",
        "//types:types_test",
        "//weeder:weeder_test",
    ],
)
