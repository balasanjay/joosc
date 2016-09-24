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
