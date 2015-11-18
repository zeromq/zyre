package org.zeromq.zyre;

import org.junit.Assert;
import org.junit.Test;

public class ZyreTest {
    @Test
    public void testZyre() {
        Zyre zyre = new Zyre("hello");
        Assert.assertTrue(zyre.start());
        System.out.println("UUID: " + zyre.uuid());
        System.out.println("Name: " + zyre.name());
        zyre.stop();
        zyre.close();
    }
}



