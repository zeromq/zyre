package examples;

import org.zeromq.ZMsg;
import org.zyre.ZreInterface;

public class Sender
{
    public static void main (String[] args)
    {
        if (args.length < 2) {
            System.out.println ("Syntax: java org.zyre.examples.Sender filename virtualname");
            return;
        }
        System.out.printf ("Publishing %s as %s\n", args [0], args [1]);
        ZreInterface inf = new ZreInterface ();
        inf.publish (args [0], args [1]);
        while (true) {
            ZMsg incoming = inf.recv ();
            if (incoming == null)
                break;
            incoming.dump (System.out);
            incoming.destroy ();
        }
        inf.destroy ();
    }

}
