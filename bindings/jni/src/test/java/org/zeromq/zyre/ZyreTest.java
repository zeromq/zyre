package org.zeromq.zyre;

import org.junit.Assert;
import org.junit.Test;
import org.zeromq.czmq.ZList;

public class ZyreTest {
    private static final boolean VERBOSE = true;

    @Test
    public void testZyre() throws InterruptedException {
        Zyre node1 = new Zyre("node1");
        Assert.assertEquals("node1", node1.name());
        if (VERBOSE) {
            node1.setVerbose();
        }
        node1.setHeader("X-HELLO", "World");
        boolean rc = node1.setEndpoint("inproc://zyre-node1");
        Assert.assertTrue(rc);

        node1.gossipBind("inproc://gossip-hub");

        rc = node1.start();
        Assert.assertTrue(rc);

        Zyre node2 = new Zyre("node2");
        Assert.assertEquals("node2", node2.name());

        if (VERBOSE) {
            node2.setVerbose();
        }

        //  Set inproc endpoint for this node
        //  First, try to use existing name, it'll fail
        rc = node2.setEndpoint("inproc://zyre-node1");
        Assert.assertFalse(rc);
        //  Now use available name and confirm that it succeeds
        rc = node2.setEndpoint("inproc://zyre-node2");
        Assert.assertTrue(rc);
        //  Set up gossip network for this node
        node2.gossipConnect("inproc://gossip-hub");

        rc = node2.start();
        Assert.assertTrue(rc);

        Assert.assertNotEquals(node1.uuid(), node2.uuid());

        node1.join("GLOBAL");
        node2.join("GLOBAL");

        Thread.sleep(100);

        if (VERBOSE) {
            node1.dump();
        }

        ZList peers = node1.peers();
        Assert.assertEquals(1, peers.size());
        peers.close();

        node1.join("node1 group of one");
        node2.join("node2 group of one");

        // Give them time to join their groups
        Thread.sleep(100);

        ZList ownGroups = node1.ownGroups();
        Assert.assertEquals(2, ownGroups.size());
        ownGroups.close();

        ZList peerGroups = node1.peerGroups();
        Assert.assertEquals(2, peerGroups.size());
        peerGroups.close();

        node1.close();
        node2.close();
    }
}



