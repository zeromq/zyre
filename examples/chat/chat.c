//  --------------------------------------------------------------------------
//  Example Zyre distributed chat application
//
//  --------------------------------------------------------------------------
//  Copyright (c) 2010-2014 iMatix Corporation and Contributors
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

//  This thread will listen and publish anything received
//  on the CHAT group

static void
chat_task (void *args, zctx_t *ctx, void *pipe) 
{
    zyre_t *node = zyre_new (ctx);
    zyre_start (node);
    zyre_join (node, "CHAT");
    
    zmq_pollitem_t items [] = {
        { pipe, 0, ZMQ_POLLIN, 0 },
        { zyre_socket (node), 0, ZMQ_POLLIN, 0 }
    };
    while (true) {
        if (zmq_poll (items, 2, -1) == -1)
            break;              //  Interrupted
            
        //  Activity on my pipe
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (pipe);
            zyre_shout (node, "CHAT", &msg);
        }
        //  Activity on my node handle
        if (items [1].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zyre_recv (node);
            zmsg_dump (msg);
            char *command = zmsg_popstr (msg);
            if (streq (command, "SHOUT")) {
                //  Discard sender and group name
                free (zmsg_popstr (msg));
                free (zmsg_popstr (msg));
                char *message = zmsg_popstr (msg);
                printf ("%s", message);
                free (message);
            }
            free (command);
            zmsg_destroy (&msg);
        }
    }
    zyre_destroy (&node);
}

int main (int argn, char *argv []) 
{
    if (argn < 2) {
        puts ("syntax: ./zyrechat myname");
        exit (0);
    }
    zctx_t *ctx = zctx_new ();
    void *chat_pipe = zthread_fork (ctx, chat_task, NULL);
    while (!zctx_interrupted) {
        char message [1024];
        if (!fgets (message, 1024, stdin))
            break;
        zstr_sendf (chat_pipe, "%s:%s", argv [1], message);
    }
    zctx_destroy (&ctx);
    return 0;
}
