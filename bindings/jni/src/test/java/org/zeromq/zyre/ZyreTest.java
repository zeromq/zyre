package org.zeromq.zyre;

import org.junit.Assert;
import org.junit.Test;

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
        node2.gossipBind("inproc://gossip-hub");

        rc = node2.start();
        Assert.assertTrue(rc);

        Assert.assertNotEquals(node1.uuid(), node2.uuid());

        node1.join("GLOBAL");
        node2.join("GLOBAL");

        Thread.sleep(100);

        if (VERBOSE) {
            node1.dump();
        }
        node1.close();
        node2.close();
    }
}



