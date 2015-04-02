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
        "//lexer",
        "//third_party/gtest",
    ],
)

cc_test(
    name = "a1",
    srcs = [
        "a1_test.cpp",
    ],
    deps = [
        "//third_party/gtest:main",
        ":common",
    ],
    data = [
        "//third_party/cs444/assignment_testcases:1",
    ],
    size = "small",
)

cc_test(
    name = "a2",
    srcs = [
        "a2_test.cpp",
    ],
    deps = [
        "//third_party/gtest:main",
        ":common",
    ],
    data = [
        "//third_party/cs444/stdlib:2",
        "//third_party/cs444/assignment_testcases:2",
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
        "//third_party/gtest:main",
        ":common",
    ],
    data = [
        "//third_party/cs444/stdlib:3",
        "//third_party/cs444/assignment_testcases:3",
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
        "//third_party/gtest:main",
        ":common",
    ],
    data = [
        "//third_party/cs444/stdlib:4",
        "//third_party/cs444/assignment_testcases:4",
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
