import unittest
from zyre import Zyre, ZyreEvent

class TestZyre(unittest.TestCase):
    def test_all(self):
        z1 = Zyre(b't1')
        z2 = Zyre(b't2')

        z1.set_header(b'test_header', b'test1')
        z2.set_header(b'test_header', b'test2')

        self.assertEquals(z1.start(), 0)
        self.assertEquals(z2.start(), 0)

        e = ZyreEvent(z1)
        self.assertEquals(e.peer_name(), z2.name())
        self.assertEquals(e.peer_uuid(), z2.uuid())
        self.assertEquals(e.type(), b'ENTER')

        e = ZyreEvent(z2)
        self.assertEquals(e.peer_name(), z1.name())
        self.assertEquals(e.peer_uuid(), z1.uuid())
        self.assertEquals(e.type(), b'ENTER')

        z1.join(b'test group')
        z2.join(b'test group')

        e = ZyreEvent(z1)
        self.assertEquals(e.peer_name(), z2.name())
        self.assertEquals(e.peer_uuid(), z2.uuid())
        self.assertEquals(e.type(), b'JOIN')
        self.assertEquals(e.group(), b'test group')

        e = ZyreEvent(z2)
        self.assertEquals(e.peer_name(), z1.name())
        self.assertEquals(e.peer_uuid(), z1.uuid())
        self.assertEquals(e.type(), b'JOIN')
        self.assertEquals(e.group(), b'test group')

        z1.shouts(b'test group', b"Hello World")

        e = ZyreEvent(z2)
        self.assertEquals(e.peer_name(), z1.name())
        self.assertEquals(e.peer_uuid(), z1.uuid())
        self.assertEquals(e.type(), b'SHOUT')
        self.assertEquals(e.group(), b'test group')
        self.assertEquals(e.msg().popstr(), b"Hello World")

if __name__ == '__main__':
    unittest.main()
