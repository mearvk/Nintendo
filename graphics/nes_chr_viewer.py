#!/usr/bin/env python3
"""Export conventional NES CHR 2bpp tiles to a grayscale PPM atlas."""
import argparse
from pathlib import Path
SHADES=(0,85,170,255)
def locate(data):
 if len(data)<16 or data[:4]!=b"NES\x1a":raise ValueError("not a complete iNES/NES 2.0 image")
 fmt=data[7]&12
 if fmt==8 and ((data[9]&15)==15 or (data[9]>>4)==15):raise ValueError("NES 2.0 exponent-sized ROMs are unsupported")
 if fmt not in (0,8):raise ValueError("unsupported/reserved header format")
 prg=data[4]|(((data[9]&15)<<8) if fmt==8 else 0); banks=data[5]|(((data[9]>>4)<<8) if fmt==8 else 0)
 if not banks:raise ValueError("image has CHR RAM, not embedded CHR ROM")
 start=16+(512 if data[6]&4 else 0)+prg*0x4000; size=banks*0x2000
 if start+size>len(data):raise ValueError("truncated CHR data")
 return start,size
def decode(raw,count=None):
 if len(raw)%16:raise ValueError("CHR data length must be a multiple of 16")
 n=len(raw)//16 if count is None else count
 if not 1<=n<=len(raw)//16:raise ValueError("invalid tile count")
 cols=min(16,n); rows=(n+cols-1)//cols; w,h=cols*8,rows*8; pixels=bytearray(w*h*3)
 for i in range(n):
  tile=raw[i*16:i*16+16]; ox=i%cols*8; oy=i//cols*8
  for y in range(8):
   lo,hi=tile[y],tile[y+8]
   for x in range(8):
    bit=7-x; shade=SHADES[((lo>>bit)&1)|(((hi>>bit)&1)<<1)]; pos=((oy+y)*w+ox+x)*3; pixels[pos:pos+3]=bytes((shade,)*3)
 return w,h,bytes(pixels)
def main():
 p=argparse.ArgumentParser(description=__doc__); p.add_argument("image",type=Path); p.add_argument("output",type=Path); p.add_argument("--tiles",type=int); a=p.parse_args()
 data=a.image.read_bytes(); start,size=locate(data); w,h,pixels=decode(data[start:start+size],a.tiles)
 a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_bytes(("P6\n%d %d\n255\n"%(w,h)).encode("ascii")+pixels); print("wrote %dx%d PPM atlas"%(w,h)); return 0
if __name__=="__main__":
 try:raise SystemExit(main())
 except (OSError,ValueError) as e:print("error: "+str(e)); raise SystemExit(2)
