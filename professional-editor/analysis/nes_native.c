#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p,0775)
#endif
static int run(const char *name,const char *root){FILE*f=fopen(name,"rb");if(!f){perror(name);return 1;}unsigned char h[16];if(fread(h,1,16,f)!=16||memcmp(h,"NES\032",4)){fprintf(stderr,"%s: invalid/truncated iNES header\n",name);fclose(f);return 1;}unsigned mapper=(h[6]>>4)|(h[7]&240);int nes2=(h[7]&12)==8;if(nes2)mapper|=(h[8]&15)<<8;unsigned long prg=(unsigned long)h[4]*16384,chr=(unsigned long)h[5]*8192;char out[2048],path[2300];const char *base=strrchr(name,'/');const char *winbase=strrchr(name,'\\\\');if(winbase&&(!base||winbase>base))base=winbase;base=base?base+1:name;snprintf(out,sizeof out,"%s/%s",root,base);MKDIR(root);MKDIR(out);snprintf(path,sizeof path,"%s/summary.md",out);FILE*s=fopen(path,"w");if(!s){perror(path);fclose(f);return 1;}fprintf(s,"# Native NES ROM inspection\n\n- Input: %s\n- Format: %s\n- Mapper: %u\n- PRG-ROM declared bytes: %lu\n- CHR-ROM declared bytes: %lu\n- Trainer: %s\n- Mirroring: %s\n\nStatic analysis only; no source ROM bytes are modified.\n",name,nes2?"NES 2.0":"iNES legacy",mapper,prg,chr,(h[6]&4)?"yes":"no",(h[6]&8)?"four-screen":(h[6]&1)?"vertical":"horizontal");fclose(s);fseek(f,0,SEEK_END);long size=ftell(f);fclose(f);printf("OK %s mapper=%u bytes=%ld output=%s\n",name,mapper,size,out);return 0;}
int main(int argc,char**argv){const char*out="rom-analysis-output";int i,bad=0;if(argc<2){fprintf(stderr,"Usage: %s [--out-dir DIR] ROM.nes ...\n",argv[0]);return 2;}for(i=1;i<argc;i++){if(!strcmp(argv[i],"--out-dir")&&i+1<argc)out=argv[++i];else bad+=run(argv[i],out);}return bad?1:0;}
