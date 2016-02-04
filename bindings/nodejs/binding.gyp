{
    "targets": [
      { 
        "target_name": "zyre",
        "sources": [
            "binding.cc",
            "../../src/zre_msg.c",
            "../../src/zyre.c",
            "../../src/zyre_event.c",
            "../../src/zyre_group.c",
            "../../src/zyre_node.c",
            "../../src/zyre_peer.c"
        ],
        "include_dirs": [
            "<!(node -e \"require('nan')\")",
            "../../include"
        ],
        "libraries": [
            "/home/ph/work/hintjens/zyre/builds/nodejs/lib/libczmq.a",
            "/home/ph/work/hintjens/zyre/builds/nodejs/lib/libzmq.a",
            "/home/ph/work/hintjens/zyre/builds/nodejs/lib/libsodium.a",
            "-luuid"
        ],
        "defines": [
            "ZYRE_BUILD_DRAFT_API",
        ]
      }
    ]
}
