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
            "lib/libzyre.a",
            "lib/libczmq.a",
            "lib/libzmq.a",
            "lib/libsodium.a",
            "-luuid"
        ],
        "defines": [
            "ZYRE_BUILD_DRAFT_API",
        ]
      }
    ]
}
