package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    srcs = [
        "common_test.cpp",
    ],
    hdrs = [
        "common_test.h",
    ],
    deps = [
        "//:joosc_lib",
        "//base",
        "//external:googletest",
        "//lexer",
    ],
)

cc_test(
    name = "a1",
    srcs = [
        "a1_test.cpp",
    ],
    deps = [
        "//external:googletest_main",
        ":common",
    ],
    data = [
        "//third_party/cs444/assignment_testcases:1",
    ],
    copts = [
        "-Wno-gnu-zero-variadic-macro-arguments",
    ],
    size = "small",
)

cc_test(
    name = "a2",
    srcs = [
        "a2_test.cpp",
    ],
    deps = [
        "//external:googletest_main",
        ":common",
    ],
    data = [
        "//third_party/cs444/stdlib:2",
        "//third_party/cs444/assignment_testcases:2",
    ],
    copts = [
        "-Wno-gnu-zero-variadic-macro-arguments",
    ],
    size = "small",
    shard_count = 4,
)

cc_test(
    name = "a3",
    srcs = [
        "a3_test.cpp",
    ],
    deps = [
        "//external:googletest_main",
        ":common",
    ],
    data = [
        "//third_party/cs444/stdlib:3",
        "//third_party/cs444/assignment_testcases:3",
    ],
    copts = [
        "-Wno-gnu-zero-variadic-macro-arguments",
    ],
    size = "small",
    shard_count = 4,
)

cc_test(
    name = "a4",
    srcs = [
        "a4_test.cpp",
    ],
    deps = [
        "//external:googletest_main",
        ":common",
    ],
    data = [
        "//third_party/cs444/stdlib:4",
        "//third_party/cs444/assignment_testcases:4",
    ],
    copts = [
        "-Wno-gnu-zero-variadic-macro-arguments",
    ],
    size = "small",
    shard_count = 4,
)

sh_test(
    name = "a5",
    srcs = [
        "a5_test.sh",
    ],
    deps = [
    ],
    data = [
        "//:asm.sh",
        "//:joosc",
        "//third_party/cs444/assignment_testcases:5",
        "//third_party/cs444/stdlib:5",
        ":a5_test.py",
    ],
    size = "small",
    shard_count = 4,
)
