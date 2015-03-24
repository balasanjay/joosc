package(default_visibility = ["//visibility:public"])

licenses(["permissive"])

cc_library(
    name = "gtest",
    srcs = ["gtest-all_test.cpp"],
    hdrs = ["gtest.h"],
)

cc_library(
    name = "main",
    srcs = [
        "gtest_main.cpp",
    ],
    deps = [
        ":gtest",
    ],
    data = [
        "//tools:test_sharding_compliant"
    ],
)
