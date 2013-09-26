/*  =========================================================================
    zre_node - node on a ZRE network

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#include <czmq.h>
#include "../include/zre_internal.h"

//  Optional global context for zre_node instances
zctx_t *zre_global_ctx = NULL;
//  Optional temp directory; set by caller if needed
char *zre_global_tmpdir = NULL;

//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_node_t {
    zctx_t *ctx;                //  Our context wrapper
    bool ctx_owned;             //  True if we created the context
    void *pipe;                 //  Pipe through to agent
};

//  =====================================================================
//  Synchronous part, works in our application thread

//  This is the thread that handles our real node class
static void
    zre_node_agent (void *args, zctx_t *ctx, void *pipe);


//  ---------------------------------------------------------------------
//  Constructor

zre_node_t *
zre_node_new (void)
{
    zre_node_t
        *self;

    self = (zre_node_t *) zmalloc (sizeof (zre_node_t));
    //  If caller set a default ctx use that, else create our own
    if (zre_global_ctx)
        self->ctx = zre_global_ctx;
    else {
        self->ctx = zctx_new ();
        self->ctx_owned = true;
    }
    self->pipe = zthread_fork (self->ctx, zre_node_agent, NULL);
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor

void
zre_node_destroy (zre_node_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_node_t *self = *self_p;
        if (self->ctx_owned)
            zctx_destroy (&self->ctx);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------
//  Receive next message from node
//  Returns zmsg_t object, or NULL if interrupted

zmsg_t *
zre_node_recv (zre_node_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_recv (self->pipe);
    return msg;
}


//  ---------------------------------------------------------------------
//  Join a group

int
zre_node_join (zre_node_t *self, const char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "JOIN");
    zstr_send  (self->pipe, group);
    return 0;
}


//  ---------------------------------------------------------------------
//  Leave a group

int
zre_node_leave (zre_node_t *self, const char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "LEAVE");
    zstr_send  (self->pipe, group);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to single peer; peer ID is first frame in message
//  Destroys message after sending

int
zre_node_whisper (zre_node_t *self, zmsg_t **msg_p)
{
    assert (self);
    zstr_sendm (self->pipe, "WHISPER");
    zmsg_send (msg_p, self->pipe);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to a group of peers

int
zre_node_shout (zre_node_t *self, zmsg_t **msg_p)
{
    assert (self);
    zstr_sendm (self->pipe, "SHOUT");
    zmsg_send (msg_p, self->pipe);
    return 0;
}


//  ---------------------------------------------------------------------
//  Return node handle, for polling

void *
zre_node_handle (zre_node_t *self)
{
    assert (self);    
    return self->pipe;
}


//  ---------------------------------------------------------------------
//  Set node header value

void
zre_node_set_header (zre_node_t *self, char *name, char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *value = (char *) malloc (255 + 1);
    vsnprintf (value, 255, format, argptr);
    va_end (argptr);
    
    zstr_sendm (self->pipe, "SET");
    zstr_sendm (self->pipe, name);
    zstr_send  (self->pipe, value);
    free (value);
}


//  ---------------------------------------------------------------------
//  Publish file under some logical name
//  Physical name is the actual file location

void
zre_node_publish (zre_node_t *self, char *logical, char *physical)
{
    zstr_sendm (self->pipe, "PUBLISH");
    zstr_sendm (self->pipe, logical);
    zstr_send  (self->pipe, physical);
}

//  ---------------------------------------------------------------------
//  Retract published file

void
zre_node_retract (zre_node_t *self, char *logical)
{
    zstr_sendm (self->pipe, "RETRACT");
    zstr_send  (self->pipe, logical);
}


//  ---------------------------------------------------------------------
//  Get temporary directory of INBOX and OUTBOX

static char *
s_tmpdir (void)
{
    static char our_tmpdir [255] = { 0 };
    if (!zre_global_tmpdir) {
        zre_global_tmpdir = our_tmpdir;
        char *tmp_env = getenv ("TMPDIR");
        if (!tmp_env)
            tmp_env = "/tmp";
        strcat (zre_global_tmpdir, tmp_env);

        if (zre_global_tmpdir [strlen (zre_global_tmpdir) -1] != '/')
            strcat (zre_global_tmpdir, "/");
        strcat (zre_global_tmpdir, "zyre");
    }
    return zre_global_tmpdir;
}


//  =====================================================================
//  Asynchronous part, works in the background

//  Beacon frame has this format:
//
//  Z R E       3 bytes
//  version     1 byte, %x01
//  UUID        16 bytes
//  port        2 bytes in network order

#define BEACON_VERSION 0x01

typedef struct {
    byte protocol [3];
    byte version;
    byte uuid [ZRE_UUID_LEN];
    uint16_t port;
} beacon_t;


//  This structure holds the context for our agent, so we can
//  pass that around cleanly to methods which need it

typedef struct {
    zctx_t *ctx;                //  CZMQ context
    void *pipe;                 //  Pipe back to application
    zbeacon_t *beacon;          //  Beacon object
    zre_log_t *log;             //  Log object
    zre_uuid_t *uuid;           //  Our UUID as object
    char *identity;             //  Our UUID as hex string
    void *inbox;                //  Our inbox socket (ROUTER)
    char *host;                 //  Our host IP address
    int port;                   //  Our inbox port number
    byte status;                //  Our own change counter
    zhash_t *peers;             //  Hash of known peers, fast lookup
    zhash_t *peer_groups;       //  Groups that our peers are in
    zhash_t *own_groups;        //  Groups that we are in
    zhash_t *headers;           //  Our header values
    
    fmq_server_t *fmq_server;   //  FileMQ server object
    int fmq_service;            //  FileMQ server port
    char fmq_outbox [255];      //  FileMQ server outbox
    
    fmq_client_t *fmq_client;   //  FileMQ client object
    char fmq_inbox [255];       //  FileMQ client inbox
} agent_t;

static agent_t *
agent_new (zctx_t *ctx, void *pipe)
{
    agent_t *self = (agent_t *) zmalloc (sizeof (agent_t));
    self->ctx = ctx;
    self->pipe = pipe;
    self->inbox = zsocket_new (ctx, ZMQ_ROUTER);
    if (self->inbox == NULL) {
        free (self);
        return NULL;            //  Interrupted 0MQ call
    }
    self->port = zsocket_bind (self->inbox, "tcp://*:*");
    if (self->port < 0) {
        free (self);
        return NULL;            //  Interrupted 0MQ call
    }
    //  Set broadcast/listen beacon
    self->uuid = zre_uuid_new ();
    self->beacon = zbeacon_new (ZRE_DISCOVERY_PORT);
    beacon_t beacon;
    beacon.protocol [0] = 'Z';
    beacon.protocol [1] = 'R';
    beacon.protocol [2] = 'E';
    beacon.version = BEACON_VERSION;
    beacon.port = htons (self->port);
    zre_uuid_cpy (self->uuid, beacon.uuid);
    zbeacon_noecho (self->beacon);
    zbeacon_publish (self->beacon, (byte *) &beacon, sizeof (beacon_t));
    zbeacon_subscribe (self->beacon, (byte *) "ZRE", 3);

    self->host = zbeacon_hostname (self->beacon);
    self->identity = strdup (zre_uuid_str (self->uuid));
    self->peers = zhash_new ();
    self->peer_groups = zhash_new ();
    self->own_groups = zhash_new ();
    self->headers = zhash_new ();
    zhash_autofree (self->headers);

    //  Set up log instance
    char endpoint [30];         //  ipaddress:port endpoint
    sprintf (endpoint, "%s:%d", self->host, self->port);
    self->log = zre_log_new (endpoint);

    //  Set up content distribution network: Each server binds to an
    //  ephemeral port and publishes a temporary directory that acts
    //  as the outbox for this node.
    //
    sprintf (self->fmq_outbox, "%s/send/%s", s_tmpdir (), self->identity);
    zfile_mkdir (self->fmq_outbox);
    sprintf (self->fmq_inbox, "%s/recv/%s", s_tmpdir (), self->identity);
    zfile_mkdir (self->fmq_inbox);

    self->fmq_server = fmq_server_new ();
    self->fmq_service = fmq_server_bind (self->fmq_server, "tcp://*:*");
    fmq_server_publish (self->fmq_server, self->fmq_outbox, "/");
    fmq_server_set_anonymous (self->fmq_server, true);
    char publisher [32];
    sprintf (publisher, "tcp://%s:%d", self->host, self->fmq_service);
    zhash_update (self->headers, "X-FILEMQ", publisher);

    //  Client will connect as it discovers new nodes
    self->fmq_client = fmq_client_new ();
    fmq_client_set_inbox (self->fmq_client, self->fmq_inbox);
    fmq_client_set_resync (self->fmq_client, true);
    fmq_client_subscribe (self->fmq_client, "/");
    
    return self;
}

static void
agent_destroy (agent_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        agent_t *self = *self_p;

        //  Destroy inbox and outbox directories if they exist
        zdir_t *inbox = zdir_new (self->fmq_inbox, NULL);
        if (inbox) {
            zdir_remove (inbox, true);
            zdir_destroy (&inbox);
        }
        zdir_t *outbox = zdir_new (self->fmq_outbox, NULL);
        if (outbox) {
            zdir_remove (outbox, true);
            zdir_destroy (&outbox);
        }
        zhash_destroy (&self->peers);
        zhash_destroy (&self->peer_groups);
        zhash_destroy (&self->own_groups);
        zhash_destroy (&self->headers);
        zbeacon_destroy (&self->beacon);
        zre_log_destroy (&self->log);
        
        fmq_server_destroy (&self->fmq_server);
        fmq_client_destroy (&self->fmq_client);
        free (self->identity);
        free (self);
        *self_p = NULL;
    }
}


//  Send message to all peers

static int
s_peer_send (const char *key, void *item, void *argument)
{
    zre_peer_t *peer = (zre_peer_t *) item;
    zre_msg_t *msg = zre_msg_dup ((zre_msg_t *) argument);
    zre_peer_send (peer, &msg);
    return 0;
}


//  Here we handle the different control messages from the front-end

static int
agent_recv_from_api (agent_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    char *command = zmsg_popstr (request);
    if (!command)
        return -1;                  //  Interrupted

    if (streq (command, "WHISPER")) {
        //  Get peer to send message to
        char *identity = zmsg_popstr (request);
        zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, identity);
        
        //  Send frame on out to peer's mailbox, drop message
        //  if peer doesn't exist (may have been destroyed)
        if (peer) {
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_WHISPER);
            zre_msg_set_content (msg, zmsg_pop (request));
            zre_peer_send (peer, &msg);
        }
        free (identity);
    }
    else
    if (streq (command, "SHOUT")) {
        //  Get group to send message to
        char *name = zmsg_popstr (request);
        zre_group_t *group = (zre_group_t *) zhash_lookup (self->peer_groups, name);
        if (group) {
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_SHOUT);
            zre_msg_set_group (msg, name);
            zre_msg_set_content (msg, zmsg_pop (request));
            zre_group_send (group, &msg);
        }
        free (name);
    }
    else
    if (streq (command, "JOIN")) {
        char *name = zmsg_popstr (request);
        zre_group_t *group = (zre_group_t *) zhash_lookup (self->own_groups, name);
        if (!group) {
            //  Only send if we're not already in group
            group = zre_group_new (name, self->own_groups);
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_JOIN);
            zre_msg_set_group (msg, name);
            //  Update status before sending command
            zre_msg_set_status (msg, ++(self->status));
            zhash_foreach (self->peers, s_peer_send, msg);
            zre_msg_destroy (&msg);
            zre_log_info (self->log, ZRE_LOG_MSG_EVENT_JOIN, NULL, name);
        }
        free (name);
    }
    else
    if (streq (command, "LEAVE")) {
        char *name = zmsg_popstr (request);
        zre_group_t *group = (zre_group_t *) zhash_lookup (self->own_groups, name);
        if (group) {
            //  Only send if we are actually in group
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_LEAVE);
            zre_msg_set_group (msg, name);
            //  Update status before sending command
            zre_msg_set_status (msg, ++(self->status));
            zhash_foreach (self->peers, s_peer_send, msg);
            zre_msg_destroy (&msg);
            zhash_delete (self->own_groups, name);
            zre_log_info (self->log, ZRE_LOG_MSG_EVENT_LEAVE, NULL, name);
        }
        free (name);
    }
    else
    if (streq (command, "SET")) {
        char *name = zmsg_popstr (request);
        char *value = zmsg_popstr (request);
        zhash_update (self->headers, name, value);
        free (name);
        free (value);
    }
    else
    if (streq (command, "PUBLISH")) {
        char *logical = zmsg_popstr (request);
        char *physical = zmsg_popstr (request);
        //  Virtual filename must start with slash
        assert (logical [0] == '/');
        //  We create symbolic link pointing to real file
        char *symlink = malloc (strlen (logical) + 3);
        sprintf (symlink, "%s.ln", logical + 1);
        zfile_t *file = zfile_new (self->fmq_outbox, symlink);
        int rc = zfile_output (file);
        assert (rc == 0);
        fprintf (zfile_handle (file), "%s\n", physical);
        zfile_destroy (&file);
        free (symlink);
        free (logical);
        free (physical);
    }
    else
    if (streq (command, "RETRACT")) {
        char *logical = zmsg_popstr (request);
        //  Logical filename must start with slash
        assert (logical [0] == '/');
        //  We delete the symbolic link pointing to real file
        char *symlink = malloc (strlen (self->fmq_outbox) + strlen (logical) + 4);
        sprintf (symlink, "%s/%s.ln", self->fmq_outbox, logical + 1);
        zsys_file_delete (symlink);
        free (symlink);
        free (logical);
    }
    free (command);
    zmsg_destroy (&request);
    return 0;
}

//  Delete peer for a given endpoint

static int
agent_peer_purge (const char *key, void *item, void *argument)
{
    zre_peer_t *peer = (zre_peer_t *) item;
    char *endpoint = (char *) argument;
    if (streq (zre_peer_endpoint (peer), endpoint))
        zre_peer_disconnect (peer);
    return 0;
}

//  Find or create peer via its UUID string

static zre_peer_t *
s_require_peer (agent_t *self, char *identity, char *address, uint16_t port)
{
    zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, identity);
    if (!peer) {
        //  Purge any previous peer on same endpoint
        char endpoint [100];
        snprintf (endpoint, 100, "%s:%hu", address, port);
        zhash_foreach (self->peers, agent_peer_purge, endpoint);

        peer = zre_peer_new (identity, self->peers, self->ctx);
        zre_peer_connect (peer, self->identity, endpoint);

        //  Handshake discovery by sending HELLO as first message
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
        zre_msg_set_ipaddress (msg, self->host);
        zre_msg_set_mailbox (msg, self->port);
        zre_msg_set_groups (msg, zhash_keys (self->own_groups));
        zre_msg_set_status (msg, self->status);
        zre_msg_set_headers (msg, zhash_dup (self->headers));
        zre_peer_send (peer, &msg);
        
        zre_log_info (self->log, ZRE_LOG_MSG_EVENT_ENTER,
                      zre_peer_endpoint (peer), endpoint);

        //  Now tell the caller about the peer
        zstr_sendm (self->pipe, "ENTER");
        zstr_send (self->pipe, identity);
    }
    return peer;
}


//  Find or create group via its name

static zre_group_t *
s_require_peer_group (agent_t *self, char *name)
{
    zre_group_t *group = (zre_group_t *) zhash_lookup (self->peer_groups, name);
    if (!group)
        group = zre_group_new (name, self->peer_groups);
    return group;
}

static zre_group_t *
s_join_peer_group (agent_t *self, zre_peer_t *peer, char *name)
{
    zre_group_t *group = s_require_peer_group (self, name);
    zre_group_join (group, peer);

    //  Now tell the caller about the peer joined group
    zstr_sendm (self->pipe, "JOIN");
    zstr_sendm (self->pipe, zre_peer_identity (peer));
    zstr_send (self->pipe, name);

    return group;
}

static zre_group_t *
s_leave_peer_group (agent_t *self, zre_peer_t *peer, char *name)
{
    zre_group_t *group = s_require_peer_group (self, name);
    zre_group_leave (group, peer);

    //  Now tell the caller about the peer left group
    zstr_sendm (self->pipe, "LEAVE");
    zstr_sendm (self->pipe, zre_peer_identity (peer));
    zstr_send (self->pipe, name);

    return group;
}

//  Here we handle messages coming from other peers

static int
agent_recv_from_peer (agent_t *self)
{
    //  Router socket tells us the identity of this peer
    zre_msg_t *msg = zre_msg_recv (self->inbox);
    if (msg == NULL)
        return 0;               //  Interrupted

    char *identity = zframe_strdup (zre_msg_address (msg));
        
    //  On HELLO we may create the peer if it's unknown
    //  On other commands the peer must already exist
    zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, identity);
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        peer = s_require_peer (
            self, identity, zre_msg_ipaddress (msg), zre_msg_mailbox (msg));
        assert (peer);
        zre_peer_set_ready (peer, true);
    }
    //  Ignore command if peer isn't ready
    if (peer == NULL || !zre_peer_ready (peer)) {
				free(identity);
        zre_msg_destroy (&msg);
        return 0;
    }
    if (!zre_peer_check_message (peer, msg)) {
        zclock_log ("W: [%s] lost messages from %s", self->identity, identity);
        assert (false);
    }

    //  Now process each command
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        //  Join peer to listed groups
        char *name = zre_msg_groups_first (msg);
        while (name) {
            s_join_peer_group (self, peer, name);
            name = zre_msg_groups_next (msg);
        }
        //  Hello command holds latest status of peer
        zre_peer_set_status (peer, zre_msg_status (msg));
        
        //  Store peer headers for future reference
        zre_peer_set_headers (peer, zre_msg_headers (msg));

        //  If peer is a ZRE/LOG collector, connect to it
        char *collector = zre_msg_headers_string (msg, "X-ZRELOG", NULL);
        if (collector)
            zre_log_connect (self->log, collector);
        
        //  If peer is a FileMQ publisher, connect to it
        char *publisher = zre_msg_headers_string (msg, "X-FILEMQ", NULL);
        if (publisher)
            fmq_client_connect (self->fmq_client, publisher);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_WHISPER) {
        //  Pass up to caller API as WHISPER event
        zframe_t *cookie = zre_msg_content (msg);
        zstr_sendm (self->pipe, "WHISPER");
        zstr_sendm (self->pipe, identity);
        zframe_send (&cookie, self->pipe, ZFRAME_REUSE); // let msg free the frame
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_SHOUT) {
        //  Pass up to caller as SHOUT event
        zframe_t *cookie = zre_msg_content (msg);
        zstr_sendm (self->pipe, "SHOUT");
        zstr_sendm (self->pipe, identity);
        zstr_sendm (self->pipe, zre_msg_group (msg));
        zframe_send (&cookie, self->pipe, ZFRAME_REUSE); // let msg free the frame
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_PING) {
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING_OK);
        zre_peer_send (peer, &msg);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_JOIN) {
        s_join_peer_group (self, peer, zre_msg_group (msg));
        assert (zre_msg_status (msg) == zre_peer_status (peer));
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_LEAVE) {
        s_leave_peer_group (self, peer, zre_msg_group (msg));
        assert (zre_msg_status (msg) == zre_peer_status (peer));
    }
    free (identity);
    zre_msg_destroy (&msg);
    
    //  Activity from peer resets peer timers
    zre_peer_refresh (peer);
    return 0;
}

//  Handle beacon

static int
agent_recv_beacon (agent_t *self)
{
    //  Get IP address and beacon of peer
    char *ipaddress = zstr_recv (zbeacon_socket (self->beacon));
    zframe_t *frame = zframe_recv (zbeacon_socket (self->beacon));

    //  Ignore anything that isn't a valid beacon
    bool is_valid = true;
    beacon_t beacon;
    if (zframe_size (frame) == sizeof (beacon_t)) {
        memcpy (&beacon, zframe_data (frame), zframe_size (frame));
        if (beacon.version != BEACON_VERSION)
            is_valid = false;
    }
    else
        is_valid = false;

    //  Check that the peer, identified by its UUID, exists
    if (is_valid) {
        zre_uuid_t *uuid = zre_uuid_new ();
        zre_uuid_set (uuid, beacon.uuid);
        zre_peer_t *peer = s_require_peer (
            self, zre_uuid_str (uuid), ipaddress, ntohs (beacon.port));
        zre_peer_refresh (peer);
        zre_uuid_destroy (&uuid);
    }
    free (ipaddress);
    zframe_destroy (&frame);
    return 0;
}


//  Forward event from fmq_client to caller

static int
agent_recv_fmq_event (agent_t *self)
{
    zmsg_t *msg = fmq_client_recv (self->fmq_client);
    if (!msg)
        return 0;
    zmsg_send (&msg, self->pipe);
    return 0;
}


//  Remove peer from group, if it's a member

static int
agent_peer_delete (const char *key, void *item, void *argument)
{
    zre_group_t *group = (zre_group_t *) item;
    zre_peer_t *peer = (zre_peer_t *) argument;
    zre_group_leave (group, peer);
    return 0;
}

//  We do this once a second:
//  - if peer has gone quiet, send TCP ping
//  - if peer has disappeared, expire it

static int
agent_ping_peer (const char *key, void *item, void *argument)
{
    agent_t *self = (agent_t *) argument;
    zre_peer_t *peer = (zre_peer_t *) item;
    char *identity = zre_peer_identity (peer);
    if (zclock_time () >= zre_peer_expired_at (peer)) {
        zre_log_info (self->log, ZRE_LOG_MSG_EVENT_EXIT,
                      zre_peer_endpoint (peer),
                      zre_peer_endpoint (peer));
        //  If peer has really vanished, expire it
        zstr_sendm (self->pipe, "EXIT");
        zstr_send (self->pipe, identity);
        zhash_foreach (self->peer_groups, agent_peer_delete, peer);
        zhash_delete (self->peers, identity);
    }
    else
    if (zclock_time () >= zre_peer_evasive_at (peer)) {
        //  If peer is being evasive, force a TCP ping.
        //  TODO: do this only once for a peer in this state;
        //  it would be nicer to use a proper state machine
        //  for peer management.
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING);
        zre_peer_send (peer, &msg);
    }
    return 0;
}

//  The agent handles API commands

static void
zre_node_agent (void *args, zctx_t *ctx, void *pipe)
{
    //  Create agent instance to pass around
    agent_t *self = agent_new (ctx, pipe);
    if (!self)                  //  Interrupted
        return;

    zmq_pollitem_t pollitems [] = {
        { self->pipe, 0, ZMQ_POLLIN, 0 },
        { self->inbox, 0, ZMQ_POLLIN, 0 },
        { zbeacon_socket (self->beacon), 0, ZMQ_POLLIN, 0 },
        { fmq_client_handle (self->fmq_client), 0, ZMQ_POLLIN, 0 }
    };
    
    uint64_t reap_at = zclock_time () + REAP_INTERVAL;
    while (!zctx_interrupted) {
        long timeout = (long) (reap_at - zclock_time ());
        assert (timeout <= REAP_INTERVAL);
        if (timeout < 0)
            timeout = 0;
        if (zmq_poll (pollitems, 4, timeout * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        if (pollitems [0].revents & ZMQ_POLLIN)
            agent_recv_from_api (self);
        
        if (pollitems [1].revents & ZMQ_POLLIN)
            agent_recv_from_peer (self);

        if (pollitems [2].revents & ZMQ_POLLIN)
            agent_recv_beacon (self);

        if (pollitems [3].revents & ZMQ_POLLIN)
            agent_recv_fmq_event (self);

        if (zclock_time () >= reap_at) {
            reap_at = zclock_time () + REAP_INTERVAL;
            //  Ping all peers and reap any expired ones
            zhash_foreach (self->peers, agent_ping_peer, self);
        }
    }
    agent_destroy (&self);
}
