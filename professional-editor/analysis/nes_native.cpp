#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
namespace fs=std::filesystem;
static int run(const fs::path& name,const fs::path& root){std::ifstream f(name,std::ios::binary);if(!f){std::cerr<<name<<": cannot open\n";return 1;}std::vector<uint8_t>d((std::istreambuf_iterator<char>(f)),{});if(d.size()<16||d[0]!='N'||d[1]!='E'||d[2]!='S'||d[3]!=0x1a){std::cerr<<name<<": invalid/truncated iNES header\n";return 1;}bool nes2=(d[7]&12)==8;unsigned mapper=(d[6]>>4)|(d[7]&240);if(nes2)mapper|=(d[8]&15)<<8;uint64_t prg=uint64_t(d[4])*16384,chr=uint64_t(d[5])*8192;fs::path out=root/name.filename();out.replace_extension("");std::error_code ec;fs::create_directories(out,ec);if(ec){std::cerr<<ec.message()<<"\n";return 1;}std::ofstream s(out/"summary.md");if(!s){std::cerr<<"cannot write report\n";return 1;}s<<"# Native NES ROM inspection\n\n- Input: "<<name.string()<<"\n- Format: "<<(nes2?"NES 2.0":"iNES legacy")<<"\n- Mapper: "<<mapper<<"\n- PRG-ROM declared bytes: "<<prg<<"\n- CHR-ROM declared bytes: "<<chr<<"\n- Trainer: "<<((d[6]&4)?"yes":"no")<<"\n- Mirroring: "<<((d[6]&8)?"four-screen":(d[6]&1)?"vertical":"horizontal")<<"\n\nStatic analysis only; input ROM bytes are never modified.\n";std::cout<<"OK "<<name<<" mapper="<<mapper<<" bytes="<<d.size()<<" output="<<out<<"\n";return 0;}
int main(int argc,char**argv){fs::path out="rom-analysis-output";std::vector<fs::path>roms;for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--out-dir"&&i+1<argc)out=argv[++i];else if(!a.empty()&&a[0]=='-'){std::cerr<<"Unknown option: "<<a<<"\n";return 2;}else roms.emplace_back(a);}if(roms.empty()){std::cerr<<"Usage: nes-native-cpp [--out-dir DIR] ROM.nes ...\n";return 2;}int bad=0;for(const auto&r:roms)bad+=run(r,out);return bad?1:0;}
