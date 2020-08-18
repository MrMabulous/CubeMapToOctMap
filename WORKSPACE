workspace(name = "cubemap_to_octmap")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "gtest",
  url = "https://github.com/google/googletest/archive/release-1.8.0.zip",
  strip_prefix = "googletest-release-1.8.0",
  sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
  build_file = "@//third_party:gtest.BUILD",
)

http_archive(
    name = "openexr",
    build_file = "@//third_party:openexr.BUILD",
    strip_prefix = "openexr-2.2.0",
    urls = ["https://github.com/openexr/openexr/archive/v2.2.0.zip"],
)

http_archive(
    name = "zlib",
    build_file = "@//third_party:zlib.BUILD",
    strip_prefix = "zlib-1.2.11",
    sha256 = "f5cc4ab910db99b2bdbba39ebbdc225ffc2aa04b4057bc2817f1b94b6978cfc3",
    urls = ["https://github.com/madler/zlib/archive/v1.2.11.zip"],
)