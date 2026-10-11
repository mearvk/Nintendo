#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
int main(int ac,char**av){if(ac<3){std::cerr<<"usage: nes-chr-cpp IMAGE OUTPUT.ppm [--tiles N]\n";return 2;}unsigned long nt=0;if(ac==5&&std::string(av[3])=="--tiles")nt=std::strtoul(av[4],nullptr,10);else if(ac!=3)return 2;std::ifstream f(av[1],std::ios::binary);if(!f)return 2;std::vector<unsigned char>d((std::istreambuf_iterator<char>(f)),{});if(d.size()<16||d[0]!='N'||d[1]!='E'||d[2]!='S'||d[3]!=0x1a||!d[5]){std::cerr<<"error: invalid iNES image or no CHR ROM\n";return 2;}int fmt=d[7]&12;if(fmt!=0&&fmt!=8)return 2;if(fmt==8&&((d[9]&15)==15||(d[9]>>4)==15))return 2;size_t st=16+((d[6]&4)?512:0)+(size_t)d[4]*16384,sz=(size_t)d[5]*8192;if(st+sz>d.size())return 2;if(!nt)nt=sz/16;if(!nt||nt>sz/16)return 2;int cols=(int)(nt<16?nt:16),rows=(int)((nt+cols-1)/cols),w=cols*8,h=rows*8;std::vector<unsigned char>p((size_t)w*h*3);for(unsigned long i=0;i<nt;i++)for(int y=0;y<8;y++)for(int x=0;x<8;x++){auto*t=&d[st+(size_t)i*16];int v=((t[y]>>(7-x))&1)|(((t[y+8]>>(7-x))&1)<<1);size_t q=(((size_t)(i/cols)*8+y)*w+(i%cols)*8+x)*3;p[q]=p[q+1]=p[q+2]=(unsigned char)(v*85);}std::ofstream o(av[2],std::ios::binary);if(!o)return 2;o<<"P6\n"<<w<<" "<<h<<"\n255\n";o.write((char*)p.data(),p.size());if(!o)return 2;std::cout<<"wrote "<<w<<"x"<<h<<" PPM atlas\n";return 0;}
