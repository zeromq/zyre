/*  =========================================================================
    zre_msg - work with ZRE messages

    Codec header for zre_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zre_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef ZRE_MSG_H_INCLUDED
#define ZRE_MSG_H_INCLUDED

/*  These are the zre_msg messages:

    HELLO - Greet a peer so it can connect back to us
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        endpoint            string      Sender connect endpoint
        groups              strings     List of groups sender is in
        status              number 1    Sender groups status value
        name                string      Sender public name
        headers             hash        Sender header properties

    WHISPER - Send a multi-part message to a peer
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        content             msg         Wrapped message content

    SHOUT - Send a multi-part message to a group
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Group to send to
        content             msg         Wrapped message content

    JOIN - Join a group
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Name of group
        status              number 1    Sender groups status value

    LEAVE - Leave a group
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Name of group
        status              number 1    Sender groups status value

    PING - Ping a peer that has gone silent
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number

    PING_OK - Reply to a peer's ping
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number

    ELECT - This is the ZRE protocol, version 2 draft, as defined by rfc.zeromq.org/spec:36/ZRE.
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Name of group
        challenger_id       string      ID of the challenger

    LEADER - This is the ZRE protocol, version 2 draft, as defined by rfc.zeromq.org/spec:36/ZRE.
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Name of group
        leader_id           string      ID of the elected leader
*/


#define ZRE_MSG_HELLO                       1
#define ZRE_MSG_WHISPER                     2
#define ZRE_MSG_SHOUT                       3
#define ZRE_MSG_JOIN                        4
#define ZRE_MSG_LEAVE                       5
#define ZRE_MSG_PING                        6
#define ZRE_MSG_PING_OK                     7
#define ZRE_MSG_ELECT                       8
#define ZRE_MSG_LEADER                      9

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef ZRE_MSG_T_DEFINED
typedef struct _zre_msg_t zre_msg_t;
#define ZRE_MSG_T_DEFINED
#endif

//  @interface
//  Create a new empty zre_msg
ZYRE_PRIVATE zre_msg_t *
    zre_msg_new (void);

//  Create a new zre_msg from zpl/zconfig_t *
ZYRE_PRIVATE zre_msg_t *
    zre_msg_new_zpl (zconfig_t *config);

//  Destroy a zre_msg instance
ZYRE_PRIVATE void
    zre_msg_destroy (zre_msg_t **self_p);

//  Create a deep copy of a zre_msg instance
ZYRE_PRIVATE zre_msg_t *
    zre_msg_dup (zre_msg_t *other);

//  Receive a zre_msg from the socket. Returns 0 if OK, -1 if
//  the read was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.
ZYRE_PRIVATE int
    zre_msg_recv (zre_msg_t *self, zsock_t *input);

//  Send the zre_msg to the output socket, does not destroy it
ZYRE_PRIVATE int
    zre_msg_send (zre_msg_t *self, zsock_t *output);


//  Print contents of message to stdout
ZYRE_PRIVATE void
    zre_msg_print (zre_msg_t *self);

//  Export class as zconfig_t*. Caller is responsibe for destroying the instance
ZYRE_PRIVATE zconfig_t *
    zre_msg_zpl (zre_msg_t *self, zconfig_t* parent);

//  Get/set the message routing id
ZYRE_PRIVATE zframe_t *
    zre_msg_routing_id (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_routing_id (zre_msg_t *self, zframe_t *routing_id);

//  Get the zre_msg id and printable command
ZYRE_PRIVATE int
    zre_msg_id (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_id (zre_msg_t *self, int id);
ZYRE_PRIVATE const char *
    zre_msg_command (zre_msg_t *self);

//  Get/set the sequence field
ZYRE_PRIVATE uint16_t
    zre_msg_sequence (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_sequence (zre_msg_t *self, uint16_t sequence);

//  Get/set the endpoint field
ZYRE_PRIVATE const char *
    zre_msg_endpoint (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_endpoint (zre_msg_t *self, const char *value);

//  Get/set the groups field
ZYRE_PRIVATE zlist_t *
    zre_msg_groups (zre_msg_t *self);
//  Get the groups field and transfer ownership to caller
ZYRE_PRIVATE zlist_t *
    zre_msg_get_groups (zre_msg_t *self);
//  Set the groups field, transferring ownership from caller
ZYRE_PRIVATE void
    zre_msg_set_groups (zre_msg_t *self, zlist_t **groups_p);

//  Get/set the status field
ZYRE_PRIVATE byte
    zre_msg_status (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_status (zre_msg_t *self, byte status);

//  Get/set the name field
ZYRE_PRIVATE const char *
    zre_msg_name (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_name (zre_msg_t *self, const char *value);

//  Get a copy of the headers field
ZYRE_PRIVATE zhash_t *
    zre_msg_headers (zre_msg_t *self);
//  Get the headers field and transfer ownership to caller
ZYRE_PRIVATE zhash_t *
    zre_msg_get_headers (zre_msg_t *self);
//  Set the headers field, transferring ownership from caller
ZYRE_PRIVATE void
    zre_msg_set_headers (zre_msg_t *self, zhash_t **hash_p);

//  Get a copy of the content field
ZYRE_PRIVATE zmsg_t *
    zre_msg_content (zre_msg_t *self);
//  Get the content field and transfer ownership to caller
ZYRE_PRIVATE zmsg_t *
    zre_msg_get_content (zre_msg_t *self);
//  Set the content field, transferring ownership from caller
ZYRE_PRIVATE void
    zre_msg_set_content (zre_msg_t *self, zmsg_t **msg_p);

//  Get/set the group field
ZYRE_PRIVATE const char *
    zre_msg_group (zre_msg_t *self);
ZYRE_PRIVATE void
    zre_msg_set_group (zre_msg_t *self, const char *value);

//  Get/set the challenger_id field
ZYRE_EXPORT const char *
    zre_msg_challenger_id (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_challenger_id (zre_msg_t *self, const char *value);

//  Get/set the leader_id field
ZYRE_EXPORT const char *
    zre_msg_leader_id (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_leader_id (zre_msg_t *self, const char *value);

//  Self test of this class
ZYRE_PRIVATE void
    zre_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define zre_msg_dump        zre_msg_print

#ifdef __cplusplus
}
#endif

#endif
