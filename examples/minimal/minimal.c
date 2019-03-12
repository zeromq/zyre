//  --------------------------------------------------------------------------
//  Example Zyre distributed chat application
//
//  Copyright (c) 2019 The Zyre Authors
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  --------------------------------------------------------------------------

#include "zyre.h"

#define ROOM "GROUP"

// Minimalistic example of using zyre API:
//  start two clients, each will SHOUT 10 messages
//
// gcc `pkg-config --libs --cflags libzyre` minimal.c -o minimal
// ./minimal node1
// ./minimal node2

int
main (int argc, char *argv [])
{
    if (argc < 2) {
        puts ("syntax: ./minimal myname");
        exit (0);
    }
    const char *name = argv[1];
    zyre_t *node = zyre_new (name);
    assert(node);
    zyre_start (node);
    zyre_join (node, ROOM);

    size_t count = 0;
    while (!zsys_interrupted && count < 10) {
        zyre_shouts(node, ROOM, "Hello from %s", name);
        zclock_sleep(250);
        zmsg_t *msg = zyre_recv(node);
        zmsg_print(msg);
        zmsg_destroy (&msg);

        count ++;
    }

    zyre_stop (node);
    zyre_destroy (&node);
}
