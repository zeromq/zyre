//  Interface class for Chapter 8
//  This implements an "interface" to our network of nodes

#include <czmq.h>
#include <uuid/uuid.h>
#include "../include/zre.h"


//  =====================================================================
//  Synchronous part, works in our application thread

//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_interface_t {
    zctx_t *ctx;                //  Our context wrapper
    void *pipe;                 //  Pipe through to agent
} ;

//  This is the thread that handles our real interface class
static void
    zre_interface_agent (void *args, zctx_t *ctx, void *pipe);

//  Constructor and destructor for the interface class:

zre_interface_t *
zre_interface_new (void)
{
    zre_interface_t
        *self;

    self = (zre_interface_t *) zmalloc (sizeof (zre_interface_t));
    self->ctx = zctx_new ();
    self->pipe = zthread_fork (self->ctx, zre_interface_agent, NULL);
    return self;
}

void
zre_interface_destroy (zre_interface_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_interface_t *self = *self_p;
        zctx_destroy (&self->ctx);
        free (self);
        *self_p = NULL;
    }
}

//  Wait for message from the interface
//  Returns zmsg_t object, or NULL if interrupted

zmsg_t *
zre_interface_recv (zre_interface_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_recv (self->pipe);
    return msg;
}


//  =====================================================================
//  Asynchronous part, works in the background

#define PING_PORT_NUMBER 9999
#define PING_INTERVAL    1000  //  Once per second
#define PEER_EXPIRY      5000  //  Five seconds and it's gone


//  Convert binary UUID to freshly allocated string

char *
s_uuid_str (uuid_t uuid)
{
    char hex_char [] = "0123456789ABCDEF";
    char *string = zmalloc (sizeof (uuid_t) * 2 + 1);
    int byte_nbr;
    for (byte_nbr = 0; byte_nbr < sizeof (uuid_t); byte_nbr++) {
        string [byte_nbr * 2 + 0] = hex_char [uuid [byte_nbr] >> 4];
        string [byte_nbr * 2 + 1] = hex_char [uuid [byte_nbr] & 15];
    }
    return string;
}

//  This structure defines each peer that we discover and track

typedef struct {
    uuid_t uuid;                //  Peer's UUID as binary blob
    char *uuid_str;             //  UUID as printable string
    uint64_t expires_at;
} peer_t;

//  Construct new peer object

peer_t *
peer_new (uuid_t uuid)
{
    peer_t *self = (peer_t *) zmalloc (sizeof (peer_t));
    memcpy (self->uuid, uuid, sizeof (uuid_t));
    self->uuid_str = s_uuid_str (self->uuid);
    return self;
}

//  Destroy peer object

void
peer_destroy (peer_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        peer_t *self = *self_p;
        free (self->uuid_str);
        free (self);
        *self_p = NULL;
    }
}

//  Return peer UUID blob

byte *
peer_uuid (peer_t *self)
{
    assert (self);
    return self->uuid;
}

//  Return peer UUID string

char *
peer_uuid_str (peer_t *self)
{
    assert (self);
    return self->uuid_str;
}

//  Peer hash calls this handler automatically whenever we delete
//  peer from agent peers, or destroy that hash table.

void
peer_freefn (void *argument)
{
    peer_t *peer = (peer_t *) argument;
    peer_destroy (&peer);
}

void
peer_is_alive (peer_t *self)
{
    assert (self);
    self->expires_at = zclock_time () + PEER_EXPIRY;
}

//  This structure holds the context for our agent, so we can
//  pass that around cleanly to methods which need it

typedef struct {
    zctx_t *ctx;                //  CZMQ context
    void *pipe;                 //  Pipe back to application
    zre_udp_t *udp;             //  UDP object
    uuid_t uuid;                //  Our UUID as binary blob
    zhash_t *peers;             //  Hash of known peers, fast lookup
} agent_t;

agent_t *
agent_new (zctx_t *ctx, void *pipe)
{
    agent_t *self = (agent_t *) zmalloc (sizeof (agent_t));
    self->ctx = ctx;
    self->pipe = pipe;
    self->udp = zre_udp_new (PING_PORT_NUMBER);
    self->peers = zhash_new ();
    uuid_generate (self->uuid);
    return self;
}

void
agent_destroy (agent_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        agent_t *self = *self_p;
        zhash_destroy (&self->peers);
        zre_udp_destroy (&self->udp);
        free (self);
        *self_p = NULL;
    }
}

//  Here we handle the different control messages from the front-end;

int
agent_control_message (agent_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *msg = zmsg_recv (self->pipe);
    char *command = zmsg_popstr (msg);
    if (command == NULL)
        return -1;      //  Interrupted

    //  We don't actually implement any control commands yet
    //  but if we did, this would be where we did it...
    //  if (streq (command, "EXAMPLE")) {
    //  }
    
    free (command);
    zmsg_destroy (&msg);
    return 0;
}

//  Handle beacon

int
agent_handle_beacon (agent_t *self)
{
    uuid_t uuid;
    ssize_t size = zre_udp_recv (self->udp, uuid, sizeof (uuid_t));

    //  If we got a UUID and it's not our own beacon, we have a peer
    if (size == sizeof (uuid_t)
    &&  memcmp (uuid, self->uuid, sizeof (uuid))) {
        char *uuid_str = s_uuid_str (uuid);
        //  Find or create peer via its UUID string
        peer_t *peer = (peer_t *) zhash_lookup (self->peers, uuid_str);
        if (peer == NULL) {
            peer = peer_new (uuid);
            zhash_insert (self->peers, uuid_str, peer);
            zhash_freefn (self->peers, uuid_str, peer_freefn);
            //  Report peer joined the network
            zstr_sendm (self->pipe, "JOINED");
            zstr_send (self->pipe, uuid_str);
        }
        //  Any activity from the peer means it's alive
        peer_is_alive (peer);
        free (uuid_str);
    }
    return 0;
}

//  Reap peer if no signs of life by expiry time

int
agent_reap_peer (const char *key, void *item, void *argument)
{
    agent_t *self = (agent_t *) argument;
    peer_t *peer = (peer_t *) item;
    if (zclock_time () >= peer->expires_at) {
        //  Report peer left the network
        zstr_sendm (self->pipe, "LEFT");
        zstr_send (self->pipe, peer_uuid_str (peer));
        zhash_delete (self->peers, peer_uuid_str (peer));
    }
    return 0;
}

//  The agent handles API commands

void
zre_interface_agent (void *args, zctx_t *ctx, void *pipe)
{
    //  Create agent instance to pass around
    agent_t *self = agent_new (ctx, pipe);
    
    //  Send first beacon immediately
    uint64_t ping_at = zclock_time ();
    zmq_pollitem_t pollitems [] = {
        { self->pipe, 0, ZMQ_POLLIN, 0 },
        { 0, zre_udp_handle (self->udp), ZMQ_POLLIN, 0 }
    };
    
    while (!zctx_interrupted) {
        long timeout = (long) (ping_at - zclock_time ());
        if (timeout < 0)
            timeout = 0;
        if (zmq_poll (pollitems, 2, timeout * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        if (pollitems [0].revents & ZMQ_POLLIN)
            agent_control_message (self);
        
        if (pollitems [1].revents & ZMQ_POLLIN)
            agent_handle_beacon (self);

        if (zclock_time () >= ping_at) {
            zre_udp_send (self->udp, self->uuid, sizeof (uuid_t));
            ping_at = zclock_time () + PING_INTERVAL;
        }
        //  Delete and report any expired peers
        zhash_foreach (self->peers, agent_reap_peer, self);
    }
    agent_destroy (&self);
}
