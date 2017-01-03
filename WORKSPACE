workspace(name = "joosc")

new_git_repository(
    name = "googletest_repo",
    remote = "https://github.com/google/googletest.git",
    commit = "f570b27e15a4e921d59495622a82277a3e1e8f87",
    build_file = "BUILD.googletest",
)

bind(
    name = "googletest",
    actual = "@googletest_repo//:googletest",
)

bind(
    name = "googletest_main",
    actual = "@googletest_repo//:googletest_main",
)

bind(
    name = "googletest_prod",
    actual = "@googletest_repo//:googletest_prod",
)

maven_jar(
    name = "junit_junit",
    artifact = "junit:junit:4.12",
    sha1 = "2973d150c0dc1fefe998f834810d68f278ea58ec",
)

bind(
    name = "junit",
    actual = "@junit_junit//jar",
)

maven_jar(
    name = "truth_truth",
    artifact = "com.google.truth:truth:0.30",
    sha1 = "9d591b5a66eda81f0b88cf1c748ab8853d99b18b",
)

bind(
    name = "truth",
    actual = "@truth_truth//jar",
)

maven_jar(
    name = "guava_guava",
    artifact = "com.google.guava:guava:20.0",
    sha1 = "89507701249388e1ed5ddcf8c41f4ce1be7831ef",
)

bind(
    name = "guava",
    actual = "@guava_guava//jar",
)
