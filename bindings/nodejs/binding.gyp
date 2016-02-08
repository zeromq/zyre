{
    "targets": [
      { 
        "target_name": "zyre",
        "sources": [
            "binding.cc"
        ],
        "variables": {
            "BUILD_ROOT": "<!(echo $BUILD_ROOT)"
        },
        "include_dirs": [
            "<!(node -e \"require('nan')\")",
            "../../include",
            "../../nodejs-build/src"
        ],
        "libraries": [
            "$(BUILD_ROOT)/deps/lib/libzyre.a",
            "$(BUILD_ROOT)/deps/lib/libczmq.a",
            "$(BUILD_ROOT)/deps/lib/libzmq.a",
            "$(BUILD_ROOT)/deps/lib/libsodium.a",
            "-luuid"
        ],
        "defines": [
            "ZYRE_BUILD_DRAFT_API",
        ]
      }
    ]
}
