/* Native fixed-length NES ROM patch builder. C99, no third-party dependencies. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

typedef struct { size_t off, n; unsigned char *expected, *replacement; } Patch;
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    return -1;
}
static unsigned char *hexbytes(const char *s, size_t *n) {
    size_t i, len = strlen(s); unsigned char *b;
    if (!len || (len & 1)) return NULL;
    b = (unsigned char*)malloc(len/2); if (!b) return NULL;
    for (i=0;i<len;i+=2) { int a=hexval(s[i]), z=hexval(s[i+1]); if(a<0||z<0){free(b);return NULL;} b[i/2]=(unsigned char)((a<<4)|z); }
    *n=len/2; return b;
}
static int same_file(const char *a,const char *b) {
    struct stat sa,sb;
    if(strcmp(a,b)==0)return 1;
    if(stat(a,&sa)==0&&stat(b,&sb)==0&&sa.st_dev==sb.st_dev&&sa.st_ino==sb.st_ino)return 1;
    return 0;
}
static int read_all(const char *p, unsigned char **d, size_t *n) {
    FILE *f=fopen(p,"rb"); long z;
    if(!f){fprintf(stderr,"%s: %s\n",p,strerror(errno));return 0;}
    if(fseek(f,0,SEEK_END)|| (z=ftell(f))<0 || fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    *n=(size_t)z; *d=(unsigned char*)malloc(*n?*n:1);
    if(!*d){fclose(f);return 0;}
    if(fread(*d,1,*n,f)!=*n){fprintf(stderr,"%s: read failed\n",p);free(*d);*d=NULL;fclose(f);return 0;}
    fclose(f);return 1;
}
static int write_atomic(const char *out,const unsigned char *d,size_t n) {
    char tmp[4096]; FILE *f; int failed;
    if(snprintf(tmp,sizeof tmp,"%s.tmp",out)>=(int)sizeof tmp){fprintf(stderr,"output path too long\n");return 0;}
    f=fopen(tmp,"wb"); if(!f){fprintf(stderr,"%s: %s\n",tmp,strerror(errno));return 0;}
    failed=(fwrite(d,1,n,f)!=n); if(fclose(f)!=0)failed=1;
    if(failed){remove(tmp);fprintf(stderr,"write failed\n");return 0;}
    if(rename(tmp,out)!=0){fprintf(stderr,"cannot rename temporary output: %s\n",strerror(errno));remove(tmp);return 0;}
    return 1;
}
int main(int argc,char **argv) {
    const char *input=NULL,*output=NULL; Patch p[1024]; size_t np=0,i,j,n=0; unsigned char *data=NULL; int bad=0;
    memset(p,0,sizeof p);
    for(i=1;i<(size_t)argc;i++) {
        if(!strcmp(argv[i],"--input")&&i+1<(size_t)argc) input=argv[++i];
        else if(!strcmp(argv[i],"--output")&&i+1<(size_t)argc) output=argv[++i];
        else if(!strcmp(argv[i],"--patch")&&i+3<(size_t)argc) {
            char *end=NULL; unsigned long long off; size_t en=0,rn=0;
            if(np==1024){fprintf(stderr,"too many patches (max 1024)\n");bad=1;break;}
            errno=0; off=strtoull(argv[++i],&end,0);
            if(errno||!end||*end||off>(unsigned long long)SIZE_MAX){fprintf(stderr,"invalid patch offset\n");bad=1;break;}
            p[np].off=(size_t)off; p[np].expected=hexbytes(argv[++i],&en); p[np].replacement=hexbytes(argv[++i],&rn);
            if(!p[np].expected||!p[np].replacement||en!=rn){fprintf(stderr,"patch hex must be valid, non-empty, and equal length\n");bad=1;break;}
            p[np].n=en; np++;
        } else if(!strcmp(argv[i],"--help")||!strcmp(argv[i],"-h")) {
            puts("Usage: nes-rom-build-c --input ORIGINAL.nes --output MODIFIED.nes [--patch OFFSET EXPECTED_HEX REPLACEMENT_HEX ...]\nOffsets use decimal or 0x hexadecimal. Patch bytes must have equal lengths. Header bytes [0,16) are protected.");
            return 0;
        } else { fprintf(stderr,"unknown or incomplete argument: %s\n",argv[i]);bad=1;break; }
    }
    if(bad||!input||!output){fprintf(stderr,"Usage: %s --input ORIGINAL.nes --output MODIFIED.nes [--patch OFFSET EXPECTED_HEX REPLACEMENT_HEX ...]\n",argv[0]);return 2;}
    if(same_file(input,output)){fprintf(stderr,"input and output resolve to the same file\n");bad=1;goto done;}
    if(!read_all(input,&data,&n)) {bad=1;goto done;}
    if(n<16||memcmp(data,"NES\032",4)){fprintf(stderr,"invalid or truncated iNES header\n");bad=1;goto done;}
    for(i=0;i<np;i++) {
        if(p[i].off<16){fprintf(stderr,"patch %lu touches protected header bytes\n",(unsigned long)i);bad=1;break;}
        if(p[i].off>n||p[i].n>n-p[i].off){fprintf(stderr,"patch %lu is outside ROM bounds\n",(unsigned long)i);bad=1;break;}
        for(j=0;j<i;j++) if(p[i].off<p[j].off+p[j].n&&p[j].off<p[i].off+p[i].n){fprintf(stderr,"overlapping patches %lu and %lu\n",(unsigned long)j,(unsigned long)i);bad=1;break;}
        if(bad)break;
        if(memcmp(data+p[i].off,p[i].expected,p[i].n)){fprintf(stderr,"patch %lu expected bytes do not match baseline\n",(unsigned long)i);bad=1;break;}
    }
    if(!bad) {
        for(i=0;i<np;i++)memcpy(data+p[i].off,p[i].replacement,p[i].n);
        if(!write_atomic(output,data,n))bad=1;
        else printf("OK rebuilt %s (%lu bytes, %lu fixed-length patch(es)); input unchanged\n",output,(unsigned long)n,(unsigned long)np);
    }
done:
    free(data); for(i=0;i<np;i++){free(p[i].expected);free(p[i].replacement);} return bad?1:0;
}
