import unittest
from zyre import Zyre, ZyreEvent

class TestZyre(unittest.TestCase):
    def test_all(self):
        z1 = Zyre('t1')
        z2 = Zyre('t2')

        z1.set_header('test_header', 'test1')
        z2.set_header('test_header', 'test2')

        self.assertEquals(z1.start(), 0)
        self.assertEquals(z2.start(), 0)

        e = ZyreEvent(z1)
        self.assertEquals(e.peer_name(), z2.name())
        self.assertEquals(e.peer_uuid(), z2.uuid())
        self.assertEquals(e.type(), 'ENTER')

        e = ZyreEvent(z2)
        self.assertEquals(e.peer_name(), z1.name())
        self.assertEquals(e.peer_uuid(), z1.uuid())
        self.assertEquals(e.type(), 'ENTER')

        z1.join('test group')
        z2.join('test group')

        e = ZyreEvent(z1)
        self.assertEquals(e.peer_name(), z2.name())
        self.assertEquals(e.peer_uuid(), z2.uuid())
        self.assertEquals(e.type(), 'JOIN')
        self.assertEquals(e.group(), 'test group')

        e = ZyreEvent(z2)
        self.assertEquals(e.peer_name(), z1.name())
        self.assertEquals(e.peer_uuid(), z1.uuid())
        self.assertEquals(e.type(), 'JOIN')
        self.assertEquals(e.group(), 'test group')

        z1.shouts('test group', "Hello World")

        e = ZyreEvent(z2)
        self.assertEquals(e.peer_name(), z1.name())
        self.assertEquals(e.peer_uuid(), z1.uuid())
        self.assertEquals(e.type(), 'SHOUT')
        self.assertEquals(e.group(), 'test group')
        self.assertEquals(e.msg().popstr(), "Hello World")

if __name__ == '__main__':
    unittest.main()
