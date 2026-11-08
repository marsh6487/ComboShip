"""Inspect native OoT V2 soundfont entries without redistributing audio."""
import struct, zipfile
class Reader:
 def __init__(self,b):self.b=b;self.p=64
 def read(self,fmt):
  v=struct.unpack_from('<'+fmt,self.b,self.p);self.p+=struct.calcsize('<'+fmt);return v[0] if len(v)==1 else v
 def skip(self,n):self.p+=n
 def string(self):
  n=self.read('I');s=self.b[self.p:self.p+n].decode();self.p+=n;return s
 def sound(self):
  if not self.read('B'):return None
  self.skip(1);return self.string(),self.read('f')
def font(b):
 r=Reader(b);r.skip(12);nd,ni,ns=r.read('III')
 for _ in range(nd):
  r.skip(3);r.skip(r.read('I')*4);r.skip(1);r.string();r.skip(4)
 for _ in range(ni):
  r.skip(5);r.skip(r.read('I')*4)
  for _ in range(3):r.sound()
 return [r.sound() for _ in range(ns)]
if __name__=='__main__':
 import sys
 z=zipfile.ZipFile(sys.argv[1]);f=font(z.read('audio/fonts/00_Sound_Effects_1'))
 for i,entry in enumerate(f[:144]):
  if entry: print(hex(i),*entry)
