/*  =========================================================================
    zyre - API wrapping one Zyre node

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    Zyre does local area discovery and clustering. A Zyre node broadcasts
    UDP beacons, and connects to peers that it finds. This class wraps a
    Zyre node with a message-based API.

    All incoming events are zmsg_t messages delivered via the zyre_recv
    call. The first frame defines the type of the message, and following
    frames provide further values:

        ENTER fromnode name headers ipaddress:port
            a new peer has entered the network
        EVASIVE fromnode name
            a peer is being evasive (quiet for too long)
        EXIT fromnode name
            a peer has left the network
        JOIN fromnode name groupname
            a peer has joined a specific group
        LEAVE fromnode name groupname
            a peer has joined a specific group
        WHISPER fromnode name message
            a peer has sent this node a message
        SHOUT fromnode name groupname message
            a peer has sent one of our groups a message

    In SHOUT and WHISPER the message is zero or more frames, and can hold
    any ZeroMQ message. In ENTER, the headers frame contains a packed
    dictionary, see zhash_pack/unpack.

    To join or leave a group, use the zyre_join and zyre_leave methods.
    To set a header value, use the zyre_set_header method. To send a message
    to a single peer, use zyre_whisper. To send a message to a group, use
    zyre_shout.
@discuss
    Todo: allow multipart contents
@end
*/

#include "zyre_classes.h"

//  --------------------------------------------------------------------------
//  Structure of our class

struct _zyre_t {
    zactor_t *actor;            //  A Zyre instance wraps the actor instance
    zsock_t *inbox;             //  Receives incoming cluster traffic
    char *uuid;                 //  Copy of node UUID string
    char *name;                 //  Copy of node name
    char *endpoint;             //  Copy of last endpoint bound to
};


//  --------------------------------------------------------------------------
//  Constructor, creates a new Zyre node. Note that until you start the
//  node it is silent and invisible to other nodes on the network.
//  The node name is provided to other nodes during discovery. If you
//  specify NULL, Zyre generates a randomized node name from the UUID.

zyre_t *
zyre_new (const char *name)
{
    zyre_t *self = (zyre_t *) zmalloc (sizeof (zyre_t));
    assert (self);

    //  Create front-to-back pipe pair for data traffic
    zsock_t *outbox;
    self->inbox = zsys_create_pipe (&outbox);

    //  Start node engine and wait for it to be ready
    self->actor = zactor_new (zyre_node_actor, outbox);
    assert (self->actor);

    //  Send name, if any, to node ending
    if (name)
        zstr_sendx (self->actor, "SET NAME", name, NULL);

    return self;
}


//  --------------------------------------------------------------------------
//  Destructor, destroys a Zyre node. When you destroy a node, any
//  messages it is sending or receiving will be discarded.

void
zyre_destroy (zyre_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_t *self = *self_p;
        zactor_destroy (&self->actor);
        zsock_destroy (&self->inbox);
        zstr_free (&self->uuid);
        zstr_free (&self->name);
        zstr_free (&self->endpoint);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Return our node UUID string, after successful initialization

const char *
zyre_uuid (zyre_t *self)
{
    assert (self);
    //  Hold uuid string in zyre object so caller gets a safe reference
    zstr_free (&self->uuid);
    zstr_sendx (self->actor, "UUID", NULL);
    self->uuid = zstr_recv (self->actor);
    return self->uuid;
}


//  --------------------------------------------------------------------------
//  Return our node name, after successful initialization. By default
//  is taken from the UUID and shortened.

const char *
zyre_name (zyre_t *self)
{
    assert (self);
    //  Hold name in zyre object so caller gets a safe reference
    zstr_free (&self->name);
    zstr_sendx (self->actor, "NAME", NULL);
    self->name = zstr_recv (self->actor);
    return self->name;
}


//  --------------------------------------------------------------------------
//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.

void
zyre_set_name (zyre_t *self, const char *name)
{
    assert (self);
    assert (name);
    zstr_sendx (self->actor, "SET NAME", name, NULL);
}


//  --------------------------------------------------------------------------
//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.

void
zyre_set_header (zyre_t *self, const char *name, const char *format, ...)
{
    assert (self);
    assert (name);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "SET HEADER", name, string, NULL);
    zstr_free (&string);
}


//  --------------------------------------------------------------------------
//  Set verbose mode; this tells the node to log all traffic as well as
//  all major events.

void
zyre_set_verbose (zyre_t *self)
{
    assert (self);
    zstr_sendx (self->actor, "SET VERBOSE", "1", NULL);
}


//  --------------------------------------------------------------------------
//  Set UDP beacon discovery port; defaults to 5670, this call overrides
//  that so you can create independent clusters on the same network, for
//  e.g. development vs. production. Has no effect after zyre_start().

void
zyre_set_port (zyre_t *self, int port_nbr)
{
    assert (self);
    zstr_sendm (self->actor, "SET PORT");
    zstr_sendf (self->actor, "%d", port_nbr);
}


//  --------------------------------------------------------------------------
//  Set TCP ephemeral port for beacon; defaults to 0, and the port is random.
//  This call overrides this to bypass some firewall issues with random ports.
//  Has no effect after zyre_start().

void
zyre_set_beacon_peer_port (zyre_t *self, int port)
{
    assert (self);
    zstr_sendm (self->actor, "SET EPHEMERAL PORT");
    zstr_sendf (self->actor, "%d", port);
}


//  --------------------------------------------------------------------------
//  Set the node evasiveness timeout, in milliseconds. Default is 5000.
//  This can be tuned in order to deal with expected network conditions
//  and the response time expected by the application. This is tied to
//  the beacon interval and rate of messages received.
void
zyre_set_evasive_timeout (zyre_t *self, int interval)
{
    assert (self);
    zstr_sendm (self->actor, "SET EVASIVE TIMEOUT");
    zstr_sendf (self->actor, "%d", interval);
}

//  --------------------------------------------------------------------------
//  Set the node expiration timeout, in milliseconds. Default is 30000.
//  This can be tuned in order to deal with expected network conditions
//  and the response time expected by the application. This is tied to
//  the beacon interval and rate of messages received.
void
zyre_set_expired_timeout (zyre_t *self, int interval)
{
    assert (self);
    zstr_sendm (self->actor, "SET EXPIRED TIMEOUT");
    zstr_sendf (self->actor, "%d", interval);
}

//  --------------------------------------------------------------------------
//  Set UDP beacon discovery interval, in milliseconds. Default is instant
//  beacon exploration followed by pinging every 1,000 msecs.

void
zyre_set_interval (zyre_t *self, size_t interval)
{
    assert (self);
    zstr_sendm (self->actor, "SET INTERVAL");
    zstr_sendf (self->actor, "%zd", interval);
}


//  --------------------------------------------------------------------------
//  Set network interface for UDP beacons. If you do not set this, CZMQ will
//  choose an interface for you. On boxes with several interfaces you should
//  specify which one you want to use, or strange things can happen.

void
zyre_set_interface (zyre_t *self, const char *value)
{
    assert (self);
    assert (value);

    //  Implemented by zsys global for now
    zsys_set_interface (value);
}


//  --------------------------------------------------------------------------
//  By default, Zyre binds to an ephemeral TCP port and broadcasts the local
//  host name using UDP beaconing. When you call this method, Zyre will use
//  gossip discovery instead of UDP beaconing. You MUST set-up the gossip
//  service separately using zyre_gossip_bind() and _connect(). Note that the
//  endpoint MUST be valid for both bind and connect operations. You can use
//  inproc://, ipc://, or tcp:// transports (for tcp://, use an IP address
//  that is meaningful to remote as well as local nodes). Returns 0 if
//  the bind was successful, else -1.

int
zyre_set_endpoint (zyre_t *self, const char *format, ...)
{
    assert (self);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "SET ENDPOINT", string, NULL);
    free (string);
    return zsock_wait (self->actor) == 0? 0: -1;
}

//  This options enables a peer to actively contest for leadership in the
//  given group. If this option is not set the peer will still participate in
//  elections but never gets elected. This ensures that a consent for a leader
//  is reached within a group even though not every peer is contesting for
//  leadership.

void
zyre_set_contest_in_group (zyre_t *self, const char *group) {
    assert (self);
    assert (group);
    zstr_sendx (self->actor, "SET CONTEST" , group, NULL);
}

#ifdef ZYRE_BUILD_DRAFT_API
//  DRAFT-API: Public IP
void
zyre_set_advertised_endpoint (zyre_t *self, const char *endpoint)
{
    assert (self);
    assert (endpoint);

    zstr_sendx (self->actor, "SET ADVERTISED ENDPOINT", endpoint, NULL);
}
#endif

void zyre_set_zcert(zyre_t *self, zcert_t *zcert)
{
    assert (self);
    assert (zcert);

    // actor will assert check the keys
    zstr_sendx (self->actor, "SET PUBLICKEY", zcert_public_txt(zcert), NULL);
    zstr_sendx (self->actor, "SET SECRETKEY", zcert_secret_txt(zcert), NULL);
}

void zyre_set_zap_domain(zyre_t *self, const char *domain)
{
    assert (self);
    assert (domain);

    zstr_sendx (self->actor, "ZAP DOMAIN", domain, NULL);
}

//  Explicitly connect to a peer

int
zyre_require_peer (zyre_t *self, const char *uuid, const char *endpoint, const char *public_key)
{
    assert (self);
    assert (uuid);
    assert (endpoint);
    assert (public_key);

    return zstr_sendx (self->actor, "REQUIRE PEER", uuid, endpoint, public_key, NULL);
}

//  --------------------------------------------------------------------------
//  Set-up gossip discovery of other nodes. At least one node in the cluster
//  must bind to a well-known gossip endpoint, so other nodes can connect to
//  it. Note that gossip endpoints are completely distinct from Zyre node
//  endpoints, and should not overlap (they can use the same transport).

void
zyre_gossip_bind (zyre_t *self, const char *format, ...)
{
    assert (self);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "GOSSIP BIND", string, NULL);
    free (string);
}


//  --------------------------------------------------------------------------
//  Set-up gossip discovery of other nodes. A node may connect to multiple
//  other nodes, for redundancy paths. For details of the gossip network
//  design, see the CZMQ zgossip class.

void
zyre_gossip_connect (zyre_t *self, const char *format, ...)
{
    assert (self);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "GOSSIP CONNECT", string, NULL);
    free (string);
}

void
zyre_gossip_connect_curve (zyre_t *self, const char *public_key, const char *format, ...)
{
    assert (self);
    assert (public_key);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "GOSSIP CONNECT", string, public_key, NULL);
    free (string);
}

//  --------------------------------------------------------------------------
//  Inform gossip to remove a node from it's master (tuples) list
//  Useful when tracking nodes activity across the mesh

void
zyre_gossip_unpublish (zyre_t *self, const char *node)
{
    assert (self);
    assert (node);

    zstr_sendx (self->actor, "GOSSIP UNPUBLISH", node, NULL);
}


//  --------------------------------------------------------------------------
//  Start node, after setting header values. When you start a node it
//  begins discovery and connection. Returns 0 if OK, -1 if it wasn't
//  possible to start the node. If you want to use gossip discovery, set
//  the endpoint (optionally), then bind/connect the gossip network, and
//  only then start the node.

int
zyre_start (zyre_t *self)
{
    assert (self);

    zstr_sendx (self->actor, "START", NULL);
    return zsock_wait (self->actor) == 0? 0: -1;
}


//  --------------------------------------------------------------------------
//  Stop node; this signals to other peers that this node will go away.
//  This is polite; however you can also just destroy the node without
//  stopping it.

void
zyre_stop (zyre_t *self)
{
    assert (self);

    zstr_sendx (self->actor, "STOP", NULL);
    zsock_wait (self->actor);
}


//  --------------------------------------------------------------------------
//  Join a named group; after joining a group you can send messages to
//  the group and all Zyre nodes in that group will receive them.

int
zyre_join (zyre_t *self, const char *group)
{
    assert (self);
    assert (group);

    zstr_sendx (self->actor, "JOIN", group, NULL);
    return 0;
}


//  --------------------------------------------------------------------------
//  Leave a group

int
zyre_leave (zyre_t *self, const char *group)
{
    assert (self);
    assert (group);

    zstr_sendx (self->actor, "LEAVE", group, NULL);
    return 0;
}


//  --------------------------------------------------------------------------
//  Receive next message from network; the message may be a control
//  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
//  Returns zmsg_t object, or NULL if interrupted

zmsg_t *
zyre_recv (zyre_t *self)
{
    assert (self);
    return zmsg_recv (self->inbox);
}


//  --------------------------------------------------------------------------
//  Send message to single peer, specified as a UUID string
//  Destroys message after sending

int
zyre_whisper (zyre_t *self, const char *peer, zmsg_t **msg_p)
{
    assert (self);
    assert (peer);
    assert (msg_p);

    zstr_sendm (self->actor, "WHISPER");
    zstr_sendm (self->actor, peer);
    zmsg_send (msg_p, self->actor);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send message to a named group
//  Destroys message after sending

int
zyre_shout (zyre_t *self, const char *group, zmsg_t **msg_p)
{
    assert (self);
    assert (group);
    assert (msg_p);

    zstr_sendm (self->actor, "SHOUT");
    zstr_sendm (self->actor, group);
    zmsg_send (msg_p, self->actor);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send formatted string to a single peer specified as UUID string

int
zyre_whispers (zyre_t *self, const char *peer, const char *format, ...)
{
    assert (self);
    assert (peer);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->actor, "WHISPER");
    zstr_sendm (self->actor, peer);
    zstr_send (self->actor, string);
    free (string);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send formatted string to a named group

int
zyre_shouts (zyre_t *self, const char *group, const char *format, ...)
{
    assert (self);
    assert (group);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->actor, "SHOUT");
    zstr_sendm (self->actor, group);
    zstr_send  (self->actor, string);
    free (string);
    return 0;
}


//  --------------------------------------------------------------------------
//  Return zlist of current peers. The caller owns this list and should
//  destroy it when finished with it.

zlist_t *
zyre_peers (zyre_t *self)
{
    assert (self);

    zlist_t *peers;
    zstr_send (self->actor, "PEERS");
    zsock_recv (self->actor, "p", &peers);
    return peers;
}


//  --------------------------------------------------------------------------
//  Return zlist of current peers of this group. The caller owns this list and
//  should destroy it when finished with it.

zlist_t *
zyre_peers_by_group (zyre_t *self, const char *group)
{
    assert (self);
    assert (group);

    zlist_t *peers;
    zstr_sendm (self->actor, "GROUP PEERS");
    zstr_send (self->actor, group);
    zsock_recv (self->actor, "p", &peers);
    return peers;
}


//  --------------------------------------------------------------------------
//  Return the endpoint of a connected peer. Returns empty string if
//  the peer does not exist. Caller owns the string.

char *
zyre_peer_address (zyre_t *self, const char *peer)
{
    assert (self);
    assert (peer);

    char *address;
    zstr_sendm (self->actor, "PEER ENDPOINT");
    zstr_send (self->actor, peer);
    zsock_recv (self->actor, "s", &address);
    return address;
}

//  --------------------------------------------------------------------------
//  Return the value of a header of a conected peer.  Returns null if peer
//  or key doesn't exits. Caller owns the string.

char *
zyre_peer_header_value (zyre_t *self, const char *peer, const char *name)
{
    assert (self);
    assert (peer);
    assert (name);

    zstr_sendm (self->actor, "PEER HEADER");
    zstr_sendm (self->actor, peer);
    zstr_send (self->actor, name);
    return zstr_recv (self->actor);
}

//  --------------------------------------------------------------------------
//  Return zlist of currently joined groups. The caller owns this list and
//  should destroy it when finished with it.

zlist_t *
zyre_own_groups (zyre_t *self)
{
    assert (self);

    zlist_t *groups;
    zstr_send (self->actor, "OWN GROUPS");
    zsock_recv (self->actor, "p", &groups);
    return groups;
}


//  --------------------------------------------------------------------------
//  Return zlist of groups known through connected peers. The caller owns this
//  list and should destroy it when finished with it.

zlist_t *
zyre_peer_groups (zyre_t *self)
{
    assert (self);

    zlist_t *groups;
    zstr_send (self->actor, "PEER GROUPS");
    zsock_recv (self->actor, "p", &groups);
    return groups;
}


//  --------------------------------------------------------------------------
//  Return node zsock_t socket, for direct polling of socket

zsock_t *
zyre_socket (zyre_t *self)
{
    assert (self);

    return self->inbox;
}


//  --------------------------------------------------------------------------
//  Prints zyre node information

void
zyre_print (zyre_t *self)
{
    assert (self);

    zstr_send (self->actor, "DUMP");
}


//  --------------------------------------------------------------------------
//  Return the Zyre version for run-time API detection; returns
//  major * 10000 + minor * 100 + patch, as a single integer.

uint64_t
zyre_version (void)
{
    return ZYRE_VERSION;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_test (bool verbose)
{
    printf (" * zyre: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    //  We'll use inproc gossip discovery so that this works without networking

    uint64_t version = zyre_version ();
    assert ((version / 10000) % 100 == ZYRE_VERSION_MAJOR);
    assert ((version / 100) % 100 == ZYRE_VERSION_MINOR);
    assert (version % 100 == ZYRE_VERSION_PATCH);

    //  Create two nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    assert (streq (zyre_name (node1), "node1"));
    zyre_set_header (node1, "X-HELLO", "World");
    if (verbose)
        zyre_set_verbose (node1);

    //  Set inproc endpoint for this node
    int rc = zyre_set_endpoint (node1, "inproc://zyre-node1");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    rc = zyre_start (node1);
    assert (rc == 0);

    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    assert (streq (zyre_name (node2), "node2"));
    if (verbose)
        zyre_set_verbose (node2);

    //  Set inproc endpoint for this node
    //  First, try to use existing name, it'll fail
    rc = zyre_set_endpoint (node2, "inproc://zyre-node1");
    assert (rc == -1);
    //  Now use available name and confirm that it succeeds
    rc = zyre_set_endpoint (node2, "inproc://zyre-node2");
    assert (rc == 0);

    //  Set up gossip network for this node
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    rc = zyre_start (node2);
    assert (rc == 0);
    assert (strneq (zyre_uuid (node1), zyre_uuid (node2)));

    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    //  Give time for them to interconnect
    zclock_sleep (250);
    if (verbose)
        zyre_dump (node1);

    zlist_t *peers = zyre_peers (node1);
    assert (peers);
    assert (zlist_size (peers) == 1);
    zlist_destroy (&peers);

    zyre_join (node1, "node1 group of one");
    zyre_join (node2, "node2 group of one");

    // Give them time to join their groups
    zclock_sleep (250);

    zlist_t *own_groups = zyre_own_groups (node1);
    assert (own_groups);
    assert (zlist_size (own_groups) == 2);
    zlist_destroy (&own_groups);

    zlist_t *peer_groups = zyre_peer_groups (node1);
    assert (peer_groups);
    assert (zlist_size (peer_groups) == 2);
    zlist_destroy (&peer_groups);

    char *value = zyre_peer_header_value (node2, zyre_uuid (node1), "X-HELLO");
    assert (streq (value, "World"));
    zstr_free (&value);

    //  One node shouts to GLOBAL
    zyre_shouts (node1, "GLOBAL", "Hello, World");

    //  Second node should receive ENTER, JOIN, and SHOUT
    zmsg_t *msg = zyre_recv (node2);
    assert (msg);
    char *command = zmsg_popstr (msg);
    assert (streq (command, "ENTER"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 4);
    char *peerid = zmsg_popstr (msg);
    char *name = zmsg_popstr (msg);
    assert (streq (name, "node1"));
    zstr_free (&name);
    zframe_t *headers_packed = zmsg_pop (msg);

    char *address = zmsg_popstr (msg);
    char *endpoint = zyre_peer_address (node2, peerid);
    assert (streq (address, endpoint));
    zstr_free (&peerid);
    zstr_free (&endpoint);
    zstr_free (&address);

    assert (headers_packed);
    zhash_t *headers = zhash_unpack (headers_packed);
    assert (headers);
    zframe_destroy (&headers_packed);
    assert (streq ((char *) zhash_lookup (headers, "X-HELLO"), "World"));
    zhash_destroy (&headers);
    zmsg_destroy (&msg);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 3);
    zmsg_destroy (&msg);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 3);
    zmsg_destroy (&msg);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "SHOUT"));
    zstr_free (&command);
    zmsg_destroy (&msg);

    zyre_stop (node2);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "STOP"));
    zstr_free (&command);
    zmsg_destroy (&msg);

    zyre_stop (node1);

    zyre_destroy (&node1);
    zyre_destroy (&node2);

    printf ("OK\n");

#ifdef ZYRE_BUILD_DRAFT_API
    //  DRAFT-API: Security
    if (zsys_has_curve()){

        printf (" * zyre-curve: ");
        if (verbose)
            printf ("\n");

        if (verbose)
            zsys_debug("----------------TESTING CURVE --------------");

        zactor_t *speaker = zactor_new (zbeacon, NULL);
        assert (speaker);
        if (verbose)
            zstr_sendx (speaker, "VERBOSE", NULL);

        // ensuring we have a broadcast address
        zsock_send (speaker, "si", "CONFIGURE", 9999);
        char *hostname = zstr_recv (speaker);
        if (!*hostname) {
            printf ("OK (skipping test, no UDP broadcasting)\n");
            zactor_destroy (&speaker);
            freen (hostname);
            return;
        }
        freen (hostname);
        zactor_destroy (&speaker);


        // zap setup
        zactor_t *auth = zactor_new(zauth, NULL);
        assert (auth);

        if (verbose) {
            zstr_sendx(auth, "VERBOSE", NULL);
            zsock_wait(auth);
        }

        zstr_sendx (auth, "CURVE", CURVE_ALLOW_ANY, NULL);
        zsock_wait (auth);

        zyre_t *node3 = zyre_new ("node3");
        zyre_t *node4 = zyre_new ("node4");

        assert (node3);
        assert (node4);

        if (verbose) {
            zyre_set_verbose (node3);
            zyre_set_verbose (node4);
        }

        zyre_set_zap_domain(node3, "TEST");
        zyre_set_zap_domain(node4, "TEST");

        zsock_set_rcvtimeo(node3->inbox, 10000);
        zsock_set_rcvtimeo(node4->inbox, 10000);

        zcert_t *node3_cert = zcert_new ();
        zcert_t *node4_cert = zcert_new ();

        assert (node3_cert);
        assert (node4_cert);

        zyre_set_zcert (node3, node3_cert);
        zyre_set_zcert (node4, node4_cert);

        zyre_set_header (node3, "X-PUBLICKEY", "%s", zcert_public_txt (node3_cert));
        zyre_set_header (node4, "X-PUBLICKEY", "%s", zcert_public_txt (node4_cert));

        // test beacon
        if (verbose)
            zsys_debug ("----------------TESTING BEACON----------------");

        rc = zyre_start (node3);
        assert (rc == 0);

        rc = zyre_start (node4);
        assert (rc == 0);

        zyre_join (node3, "GLOBAL");
        zyre_join (node4, "GLOBAL");

        zclock_sleep (1500);

        if (verbose) {
            zyre_dump (node3);
            zyre_dump (node4);
        }

        zyre_shouts (node3, "GLOBAL", "Hello, World");

        //  Second node should receive ENTER, JOIN, and SHOUT
        msg = zyre_recv (node4);
        assert (msg);
        command = zmsg_popstr (msg);
        assert (streq (command, "ENTER"));
        zstr_free (&command);

        char *peerid = zmsg_popstr (msg);
        assert (peerid);
        name = zmsg_popstr (msg);
        assert (streq (name, "node3"));
        zmsg_destroy (&msg);

        msg = zyre_recv (node4);
        assert (msg);
        command = zmsg_popstr (msg);
        assert (streq (command, "JOIN"));
        zstr_free (&command);
        zmsg_destroy (&msg);

        msg = zyre_recv (node4);
        assert (msg);
        command = zmsg_popstr (msg);
        assert (streq (command, "SHOUT"));
        zstr_free (&command);
        zmsg_destroy (&msg);

        zyre_leave (node3, "GLOBAL");
        zyre_leave (node4, "GLOBAL");

        zstr_free (&name);
        zstr_free (&peerid);
        zstr_free (&command);

        zyre_stop (node3);
        zyre_stop (node4);

        // give things a chance to settle...
        zclock_sleep (250);

        zyre_destroy(&node3);
        zyre_destroy(&node4);

        zcert_destroy(&node3_cert);
        zcert_destroy(&node4_cert);

        // test gossip
        if (verbose)
            zsys_debug ("----------------TESTING GOSSIP----------------");

        zyre_t *node5 = zyre_new ("node5");
        zyre_t *node6 = zyre_new ("node6");

        assert (node5);
        assert (node6);

        if (verbose) {
            zyre_set_verbose (node5);
            zyre_set_verbose (node6);
        }

        // if it takes more than 10s, something probably went terribly wrong
        zsock_set_rcvtimeo(node5->inbox, 10000);
        zsock_set_rcvtimeo(node6->inbox, 10000);

        zcert_t *node5_cert = zcert_new ();
        zcert_t *node6_cert = zcert_new ();

        assert (node5_cert);
        assert (node6_cert);

        zyre_set_zcert(node5, node5_cert);
        zyre_set_zcert(node6, node6_cert);

        zyre_set_header(node5, "X-PUBLICKEY", "%s", zcert_public_txt(node5_cert));
        zyre_set_header(node6, "X-PUBLICKEY", "%s", zcert_public_txt(node6_cert));

        // TODO- set_advertised_endpoint tests
//        zyre_set_endpoint(node5, "tcp://127.0.0.1:9001");
//        zyre_set_advertised_endpoint(node5, "tcp://localhost:9001");

        const char *gossip_cert = zcert_public_txt (node5_cert);

        // TODO- need to add zyre_gossip_port functions to get port from gossip bind(?)
        zyre_gossip_bind(node5, "tcp://127.0.0.1:9001");
        zyre_gossip_connect_curve(node6, gossip_cert, "tcp://127.0.0.1:9001");

        rc = zyre_start (node5);
        assert (rc == 0);

        rc = zyre_start (node6);
        assert (rc == 0);

        zyre_join (node5, "GLOBAL");
        zyre_join (node6, "GLOBAL");

        // give things a chance to settle...
        zclock_sleep (1500);

        if (verbose) {
            zyre_dump (node5);
            zyre_dump (node6);
        }

        zyre_shouts (node5, "GLOBAL", "Hello, World");

        //  Second node should receive ENTER, JOIN, and SHOUT
        msg = zyre_recv (node6);
        assert (msg);
        command = zmsg_popstr (msg);
        assert (streq (command, "ENTER"));
        zstr_free (&command);

        peerid = zmsg_popstr (msg);
        assert (peerid);
        name = zmsg_popstr (msg);
        zmsg_destroy (&msg);

        assert (streq (name, "node5"));
        zstr_free (&name);

        zyre_leave (node5, "GLOBAL");
        zyre_leave (node6, "GLOBAL");

        zyre_stop (node5);
        zyre_stop (node6);

        // give things a chance to settle...
        zclock_sleep (250);

        zstr_free (&peerid);

        zcert_destroy (&node5_cert);
        zcert_destroy (&node6_cert);

        zyre_destroy (&node5);
        zyre_destroy (&node6);
        zactor_destroy (&auth);

        printf ("OK\n");

    }
#endif
}
