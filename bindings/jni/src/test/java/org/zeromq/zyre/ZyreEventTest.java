package org.zeromq.zyre;

import org.junit.Assert;
import org.junit.Test;
import org.zeromq.czmq.ZMsg;

public class ZyreEventTest {
    public static boolean VERBOSE = true;

    @Test
    public void testZyreEvent() throws InterruptedException {
        Zyre node1 = new Zyre("node1");
        node1.setHeader("X-HELLO", "World");
        if (VERBOSE) {
            node1.setVerbose();
        }
        if (!node1.start()) {
            node1.close();
            System.out.println("OK (skipping test, no UDP discovery)");
            return;
        }
        node1.join("GLOBAL");

        Zyre node2 = new Zyre("node2");
        node2.setHeader("X-HELLO", "World");
        if (VERBOSE) {
            node2.setVerbose();
        }
        boolean rc = node2.start();
        Assert.assertTrue(rc);
        node2.join("GLOBAL");

        Thread.sleep(250);

        ZMsg msg = new ZMsg();


        node1.close();
        node2.close();
    }
}
