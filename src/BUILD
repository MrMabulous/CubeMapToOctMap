# Copyright(c) 2020 Matthias Bühlmann, Mabulous GmbH. http://www.mabulous.com

config_setting(
    name = "windows",
    values = {
        "crosstool_top": "//crosstools/windows",
    },
)

cc_library(
    name = "openexr_deps",
    deps = [
        "@openexr//:ilm_imf",
        "@openexr//:imath",
    ],
)

cc_binary(
    name = "cubemap_to_octmap",
    srcs = [
        "main.cc",
        "stringutils.cc"
    ],
    includes = [
        "cubemaputil.h",
        "octmaputil.h",
        "stringutils.h",
        "filter.h"
    ],
    copts = select({
            ":windows": ["/std:c++17"],
            "//conditions:default": ["-std:c++17"],
    }),
    deps = [":openexr_deps"],
    visibility = ["//visibility:public"],
)
