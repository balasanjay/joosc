package(default_visibility = ["//visibility:public"])

cc_library(
    name = "base",
    srcs = [
        "error.cpp",
        "errorlist.cpp",
        "file.cpp",
        "file_impl.cpp",
        "file_walker.cpp",
        "fileset.cpp",
        "printf.cpp",
    ],
    hdrs = [
        "algorithm.h",
        "error.h",
        "errorlist.h",
        "file.h",
        "file_impl.h",
        "file_walker.h",
        "fileset.h",
        "joos_types.h",
        "macros.h",
        "printf.h",
        "shared_ptr_vector.h",
        "unique_ptr_vector.h",
    ],
    deps = [
        "//:std",
        "//external:googletest_prod"
    ],
)

filegroup(
    name = "testdata",
    srcs = [
        "testdata/a.txt",
        "testdata/testfile.txt",
    ],
)

cc_test(
    name = "base_test",
    srcs = [
        "file_impl_test.cpp",
        "file_test.cpp",
        "fileset_test.cpp",
    ],
    deps = [
        "//external:googletest_main",
        ":base",
    ],
    data = [
        ":testdata",
    ],
    size = "small",
)
