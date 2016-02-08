{
    "targets": [
      { 
        "target_name": "zyre",
        "sources": [
            "binding.cc"
        ],
        "include_dirs": [
            "<!(node -e \"require('nan')\")",
            "../../include",
            "../../nodejs-build/src"
        ],
        "libraries": [
            "../deps/lib/libzyre.a",
            "../deps/lib/libczmq.a",
            "../deps/lib/libzmq.a",
            "../deps/lib/libsodium.a",
            "-luuid"
        ],
        "defines": [
            "ZYRE_BUILD_DRAFT_API",
        ]
      }
    ]
}
