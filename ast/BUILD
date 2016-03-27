package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ast",
    srcs = [
        "extent.cpp",
        "ids.cpp",
        "visitor.cpp",
    ],
    hdrs = [
        "ast.h",
        "ast_fwd.h",
        "extent.h",
        "ids.h",
        "print_visitor.h",
        "visitor.h",
    ],
    deps = [
        "//:std",
        "//base",
        "//lexer",
    ],
)
