{
    "targets": [
      { 
        "target_name": "zyre",
        "sources": [
            "binding.cc"
        ],
        "variables": {
            #   Do at start while PRODUCT_DIR is accurate
            "BUILD_ROOT": "<(PRODUCT_DIR)"
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
            "$(BUILD_ROOT)/deps/lib/libsodium.a"
        ],
        "defines": [
            "ZYRE_BUILD_DRAFT_API",
        ]
      }
    ]
}
