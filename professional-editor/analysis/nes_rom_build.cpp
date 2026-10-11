// Native fixed-length NES ROM patch builder. C++17, no third-party dependencies.
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
namespace fs = std::filesystem;
struct Patch { std::size_t offset{}; std::vector<std::uint8_t> expected, replacement; };
static int hexval(char c) {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return c-'a'+10;
    if(c>='A'&&c<='F') return c-'A'+10;
    return -1;
}
static bool hexbytes(const std::string& s,std::vector<std::uint8_t>& out) {
    if(s.empty()||s.size()%2) return false;
    out.clear(); out.reserve(s.size()/2);
    for(std::size_t i=0;i<s.size();i+=2){int a=hexval(s[i]),b=hexval(s[i+1]);if(a<0||b<0)return false;out.push_back(static_cast<std::uint8_t>((a<<4)|b));}
    return true;
}
static bool read_all(const fs::path& p,std::vector<std::uint8_t>& d) {
    std::ifstream f(p,std::ios::binary); if(!f){std::cerr<<p<<": cannot open input\n";return false;}
    d.assign(std::istreambuf_iterator<char>(f),{}); if(f.bad()){std::cerr<<p<<": read failed\n";return false;} return true;
}
static bool write_atomic(const fs::path& out,const std::vector<std::uint8_t>& d) {
    fs::path tmp=out; tmp += ".tmp";
    { std::ofstream f(tmp,std::ios::binary|std::ios::trunc); if(!f){std::cerr<<tmp<<": cannot create temporary output\n";return false;}
      f.write(reinterpret_cast<const char*>(d.data()),static_cast<std::streamsize>(d.size())); f.close();
      if(!f){std::error_code ec;fs::remove(tmp,ec);std::cerr<<"write failed\n";return false;} }
    std::error_code ec; fs::rename(tmp,out,ec);
    if(ec){fs::remove(tmp);std::cerr<<"cannot rename temporary output: "<<ec.message()<<"\n";return false;} return true;
}
int main(int argc,char**argv) {
    fs::path input,output; std::vector<Patch> patches;
    for(int i=1;i<argc;) {
        std::string a=argv[i++];
        if(a=="--input"&&i<argc) input=argv[i++];
        else if(a=="--output"&&i<argc) output=argv[i++];
        else if(a=="--patch"&&i+2<argc) {
            char* end=nullptr; errno=0; unsigned long long off=std::strtoull(argv[i++],&end,0);
            if(errno||!end||*end||off>std::numeric_limits<std::size_t>::max()){std::cerr<<"invalid patch offset\n";return 2;}
            Patch p; p.offset=static_cast<std::size_t>(off);
            std::string e=argv[i++],r=argv[i++];
            if(!hexbytes(e,p.expected)||!hexbytes(r,p.replacement)||p.expected.size()!=p.replacement.size()){std::cerr<<"patch hex must be valid, non-empty, and equal length\n";return 2;}
            if(patches.size()>=1024){std::cerr<<"too many patches (max 1024)\n";return 2;} patches.push_back(std::move(p));
        } else if(a=="--help"||a=="-h") {
            std::cout<<"Usage: nes-rom-build-cpp --input ORIGINAL.nes --output MODIFIED.nes [--patch OFFSET EXPECTED_HEX REPLACEMENT_HEX ...]\nOffsets are decimal or 0x hexadecimal. Header bytes [0,16) are protected.\n";return 0;
        } else {std::cerr<<"unknown or incomplete argument: "<<a<<"\n";return 2;}
    }
    if(input.empty()||output.empty()){std::cerr<<"input and output are required\n";return 2;}
    std::error_code ec;
    if(fs::absolute(input,ec)==fs::absolute(output,ec) || (fs::exists(output,ec)&&fs::equivalent(input,output,ec))){std::cerr<<"input and output paths must differ\n";return 2;}
    std::vector<std::uint8_t> data; if(!read_all(input,data))return 1;
    if(data.size()<16||data[0]!='N'||data[1]!='E'||data[2]!='S'||data[3]!=0x1a){std::cerr<<"invalid or truncated iNES header\n";return 1;}
    for(std::size_t i=0;i<patches.size();++i) {
        const auto& p=patches[i];
        if(p.offset<16){std::cerr<<"patch "<<i<<" touches protected header bytes\n";return 1;}
        if(p.offset>data.size()||p.expected.size()>data.size()-p.offset){std::cerr<<"patch "<<i<<" is outside ROM bounds\n";return 1;}
        for(std::size_t j=0;j<i;++j) if(p.offset<patches[j].offset+patches[j].expected.size()&&patches[j].offset<p.offset+p.expected.size()){std::cerr<<"overlapping patches "<<j<<" and "<<i<<"\n";return 1;}
        if(!std::equal(p.expected.begin(),p.expected.end(),data.begin()+static_cast<std::ptrdiff_t>(p.offset))){std::cerr<<"patch "<<i<<" expected bytes do not match baseline\n";return 1;}
    }
    for(const auto& p:patches)std::copy(p.replacement.begin(),p.replacement.end(),data.begin()+static_cast<std::ptrdiff_t>(p.offset));
    if(!write_atomic(output,data))return 1;
    std::cout<<"OK rebuilt "<<output.string()<<" ("<<data.size()<<" bytes, "<<patches.size()<<" fixed-length patch(es)); input unchanged\n";
    return 0;
}
