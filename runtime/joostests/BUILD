package(default_visibility = ["//visibility:public"])

java_test(
    name = "joostests",
    srcs = [
        "TypeInfoTest.java",
    ],
    test_class = "joostests.TypeInfoTest",
    deps = [
        "//external:guava",
        "//external:junit",
        "//external:truth",
        "//runtime/__joos_internal__",
    ],
    size = "small",
)
