package(default_visibility = ["//visibility:public"])

cc_library(
    name = "runtime",
    srcs = [
        ":runtime.cpp",
    ],
    hdrs = [
        "runtime.h",
    ],
    deps = [
      "//:std",
    ],
)

genrule(
    name = "runtime_cpp",
    outs = ["runtime.cpp"],
    cmd = """ (
        printf '#include "runtime/runtime.h"\n\n' && \
        printf '#include "std.h"\n\n' && \
        printf 'namespace runtime {\n\n' && \
        printf 'const string ArrayFile = R"(' && \
        cat $(location //runtime/__joos_internal__:Array.java) && \
        printf ')";\n\n' && \
        printf 'const string StringOpsFile = R"(' && \
        cat $(location //runtime/__joos_internal__:StringOps.java) && \
        printf ')";\n\n' && \
        printf 'const string StackFrameFile = R"(' && \
        cat $(location //runtime/__joos_internal__:StackFrame.java) && \
        printf ')";\n\n' && \
        printf 'const string TypeInfoFile = R"(' && \
        cat $(location //runtime/__joos_internal__:TypeInfo.java) && \
        printf ')";\n\n' && \
        printf 'const int kNumRuntimeFiles = 4;\n\n' && \
        printf '}  // namespace runtime') > $(OUTS)
    """,
    srcs = [
        "//runtime/__joos_internal__:Array.java",
        "//runtime/__joos_internal__:StackFrame.java",
        "//runtime/__joos_internal__:StringOps.java",
        "//runtime/__joos_internal__:TypeInfo.java",
    ],
)

