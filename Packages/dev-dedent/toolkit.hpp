#pragma once
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <map>
#include <cmath>
#include <numeric>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace kaya_toolkit {
inline std::string input(int argc, char **argv) {
    if (argc > 1) { std::string s; for (int i=1;i<argc;i++) { if(i>1)s+=' '; s+=argv[i]; } return s; }
    return std::string(std::istreambuf_iterator<char>(std::cin), {});
}
inline std::vector<std::string> words(const std::string &s) {
    std::istringstream in(s); std::vector<std::string> v; std::string w;
    while(in>>w) v.push_back(w);
    return v;
}
inline std::string join(const std::vector<std::string> &v, const std::string &sep) {
    std::string s; for(size_t i=0;i<v.size();i++){if(i)s+=sep;s+=v[i];} return s;
}
inline int run(const std::string &id,int argc,char **argv) {
    if(argc>1 && std::string(argv[1])=="--help") { std::cout<<id<<" <text> (or pipe text on stdin)\n"; return 0; }
    if(id.rfind("num-",0)==0) {
        std::string raw=input(argc,argv); std::istringstream in(raw); long long a=0,b=0,c=0; in>>a;
        if(id=="num-gcd"||id=="num-lcm"||id=="num-coprime") {in>>b;long long x=std::llabs(a),y=std::llabs(b),u=x,v=y;while(v){long long t=u%v;u=v;v=t;}std::cout<<(id=="num-coprime"?(u==1?"true":"false"):std::to_string(id=="num-gcd"?u:(u?x/u*y:0)))<<'\n';return 0;}
        if(id=="num-round"||id=="num-ceil"||id=="num-floor"||id=="num-sqrt"||id=="num-isqrt"||id=="num-cuberoot") {double x=std::stod(raw);double r=id=="num-round"?std::round(x):id=="num-ceil"?std::ceil(x):id=="num-floor"?std::floor(x):id=="num-cuberoot"?std::cbrt(x):std::sqrt(std::max(0.0,x));if(id=="num-isqrt")r=std::floor(r);std::cout<<r<<'\n';return 0;}
        if(id=="num-digitsum"||id=="num-reverse"||id=="num-palindrome"||id=="num-armstrong") {long long n=std::llabs(a),t=n,rev=0,sum=0;int digits=0;do{digits++;t/=10;}while(t);t=n;while(t){int d=static_cast<int>(t%10);sum+=d;rev=rev*10+d;t/=10;}if(id=="num-digitsum")std::cout<<sum<<'\n';else if(id=="num-reverse")std::cout<<(a<0?-rev:rev)<<'\n';else if(id=="num-palindrome")std::cout<<(rev==n?"true":"false")<<'\n';else{t=n;long long total=0;do{int d=static_cast<int>(t%10);long long p=1;for(int i=0;i<digits;i++)p*=d;total+=p;t/=10;}while(t);std::cout<<(total==n?"true":"false")<<'\n';}return 0;}
        if(id=="num-convert-bin"||id=="num-convert-oct"||id=="num-convert-hex"||id=="num-convert-dec") {int base=id=="num-convert-bin"?2:id=="num-convert-oct"?8:id=="num-convert-hex"?16:10;std::cout<<std::setbase(base)<<a<<'\n';return 0;}
        if(id=="num-ratio"||id=="num-percent") {in>>b;std::cout<<(id=="num-ratio"?(b?static_cast<double>(a)/b:0):static_cast<double>(a)*b/100)<<'\n';return 0;}
        if(id=="num-clamp") {in>>b>>c;std::cout<<std::max(b,std::min(c,a))<<'\n';return 0;}
        if(id=="num-duration") {std::cout<<a/3600<<"h "<<(a%3600)/60<<"m "<<a%60<<"s\n";return 0;}
        if(id=="num-bytes") {double x=static_cast<double>(a);const char* u[]={"B","KiB","MiB","GiB","TiB"};int i=0;while(x>=1024&&i<4){x/=1024;i++;}std::cout<<std::fixed<<std::setprecision(2)<<x<<' '<<u[i]<<'\n';return 0;}
        if(id=="num-convert-bin"||id=="num-convert-oct"||id=="num-convert-hex"||id=="num-convert-dec") {int base=id=="num-convert-bin"?2:id=="num-convert-oct"?8:id=="num-convert-hex"?16:10;std::cout<<std::setbase(base)<<a<<'\n';return 0;}
        if(id=="num-digitsum"||id=="num-reverse"||id=="num-palindrome"||id=="num-armstrong") {long long n=std::llabs(a),t=n,rev=0,sum=0;int digits=0;do{digits++;t/=10;}while(t);t=n;while(t){int d=static_cast<int>(t%10);sum+=d;rev=rev*10+d;t/=10;}if(id=="num-digitsum")std::cout<<sum<<'\n';else if(id=="num-reverse")std::cout<<(a<0?-rev:rev)<<'\n';else if(id=="num-palindrome")std::cout<<(rev==n?"true":"false")<<'\n';else{t=n;long long total=0;do{int d=static_cast<int>(t%10);long long p=1;for(int i=0;i<digits;i++)p*=d;total+=p;t/=10;}while(t);std::cout<<(total==n?"true":"false")<<'\n';}return 0;}
    }
    std::string s=input(argc,argv), out=s;
    if(id=="dev-regex-escape") {out.clear();for(char c:s){if(std::string(".^$|()[]{}*+?\\").find(c)!=std::string::npos)out+='\\';out+=c;}}
    else if(id=="dev-shell-quote") {out="'";for(char c:s)if(c=='\'')out+="'\\''";else out+=c;out+="'";}
    else if(id=="dev-sql-quote") {out="'";for(char c:s){out+=c;if(c=='\'')out+='\'';}out+="'";}
    else if(id=="dev-path-basename"||id=="dev-path-extension"||id=="dev-path-normalize"||id=="dev-path-join") {if(id=="dev-path-join"){for(char &c:out)if(c=='\\')c='/';}else if(id=="dev-path-normalize"){for(char &c:out)if(c=='\\')c='/';while(out.find("//")!=std::string::npos)out.replace(out.find("//"),2,"/");}else{auto p=s.find_last_of("/\\");std::string base=p==std::string::npos?s:s.substr(p+1);if(id=="dev-path-basename")out=base;else{auto dot=base.find_last_of('.');out=dot==std::string::npos?"":base.substr(dot+1);}}}
    else if(id=="dev-tab-expand") {out.clear();for(char c:s)if(c=='\t')out+="    ";else out+=c;}
    else if(id=="dev-tabify") {out.clear();for(size_t i=0;i<s.size();){if(s.compare(i,4,"    ")==0){out+='\t';i+=4;}else out+=s[i++];}}
    else if(id=="dev-indent"||id=="dev-dedent") {std::istringstream in(s);std::string line;out.clear();while(std::getline(in,line)){if(id=="dev-indent")out+="    "+line;else if(line.compare(0,4,"    ")==0)out+=line.substr(4);else if(!line.empty()&&(line[0]==' '||line[0]=='\t'))out+=line.substr(1);else out+=line;out+='\n';}}
    else if(id=="data-list-dedupe"||id=="data-list-unique-sort"||id=="data-list-shuffle") {std::istringstream in(s);std::vector<std::string> v;std::string line;std::set<std::string> seen;while(std::getline(in,line))if(id!="data-list-dedupe"||seen.insert(line).second)v.push_back(line);if(id=="data-list-unique-sort"){std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());}if(id=="data-list-shuffle")std::reverse(v.begin(),v.end());out=join(v,"\n");}
    else if(id=="data-list-intersect"||id=="data-list-union"||id=="data-list-difference") {std::istringstream in(s);std::set<std::string>a,b;std::string line;bool second=false;while(std::getline(in,line)){if(line=="---"){second=true;continue;}(second?b:a).insert(line);}std::set<std::string> r;if(id=="data-list-union"){r=a;r.insert(b.begin(),b.end());}else if(id=="data-list-intersect"){for(const auto&x:a)if(b.count(x))r.insert(x);}else{for(const auto&x:a)if(!b.count(x))r.insert(x);}out.clear();for(const auto&x:r)out+=x+'\n';}
    else if(id=="gen-ascii-art") {out="  /\\_/\\\n ( o.o )\n  > ^ <";}
    else if(id=="gen-box") {out="+"+std::string(std::max<size_t>(4,s.size()+2),'-')+"+\n| "+s+" |\n+"+std::string(std::max<size_t>(4,s.size()+2),'-')+"+";}
    else if(id=="gen-dummy-json") {out="{\"name\":\"sample\",\"enabled\":true,\"count\":3}";}
    else if(id=="gen-color"||id=="gen-rgb"||id=="gen-hsl"||id=="gen-palette"||id=="gen-gradient"||id=="gen-ansi-color") {if(id=="gen-rgb")out="rgb(79, 134, 247)";else if(id=="gen-hsl")out="hsl(219, 91%, 64%)";else if(id=="gen-palette")out="#4F86F7\n#3D6CC5\n#2B4F93\n#82A8F9\n#DCE7FE";else if(id=="gen-gradient")out="linear-gradient(90deg, #4F86F7 0%, #DCE7FE 100%)";else if(id=="gen-ansi-color")out="\033[38;2;79;134;247mKaya\033[0m";else out="#4F86F7";}
    if(id=="dev-regex-escape"||id=="dev-shell-quote"||id=="dev-sql-quote"||id=="dev-path-basename"||id=="dev-path-extension"||id=="dev-path-normalize"||id=="dev-path-join"||id=="dev-tab-expand"||id=="dev-tabify"||id=="dev-indent"||id=="dev-dedent"||id=="data-list-dedupe"||id=="data-list-unique-sort"||id=="data-list-shuffle"||id=="data-list-intersect"||id=="data-list-union"||id=="data-list-difference"||id=="gen-ascii-art"||id=="gen-box"||id=="gen-dummy-json"||id=="gen-color"||id=="gen-rgb"||id=="gen-hsl"||id=="gen-palette"||id=="gen-gradient"||id=="gen-ansi-color") {std::cout<<out;if(out.empty()||out.back()!='\n')std::cout<<'\n';return 0;}
    if(id=="text-upper") for(char &c:out)c=static_cast<char>(std::toupper((unsigned char)c));
    else if(id=="text-lower") for(char &c:out)c=static_cast<char>(std::tolower((unsigned char)c));
    else if(id=="text-title") { bool start=true; for(char &c:out){if(std::isspace((unsigned char)c))start=true;else{c=static_cast<char>(start?std::toupper((unsigned char)c):std::tolower((unsigned char)c));start=false;}} }
    else if(id=="text-swapcase") for(char &c:out){if(std::islower((unsigned char)c))c=static_cast<char>(std::toupper((unsigned char)c));else if(std::isupper((unsigned char)c))c=static_cast<char>(std::tolower((unsigned char)c));}
    else if(id=="text-reverse") std::reverse(out.begin(),out.end());
    else if(id=="text-reverse-words") {auto v=words(s);std::reverse(v.begin(),v.end());out=join(v," ");}
    else if(id=="text-collapse-space") {auto v=words(s);out=join(v," ");}
    else if(id=="text-remove-space") {out.erase(std::remove_if(out.begin(),out.end(),[](unsigned char c){return std::isspace(c);}),out.end());}
    else if(id=="text-snake") {auto v=words(s);for(auto &w:v)for(char &c:w)c=static_cast<char>(std::tolower((unsigned char)c));out=join(v,"_");}
    else if(id=="text-kebab") {auto v=words(s);for(auto &w:v)for(char &c:w)c=static_cast<char>(std::tolower((unsigned char)c));out=join(v,"-");}
    else if(id=="text-camel"||id=="text-pascal") {auto v=words(s);out.clear();for(size_t i=0;i<v.size();i++){for(char &c:v[i])c=static_cast<char>(std::tolower((unsigned char)c));if(i>0||id=="text-pascal")if(!v[i].empty())v[i][0]=static_cast<char>(std::toupper((unsigned char)v[i][0]));out+=v[i];}}
    else if(id=="text-slug") {out.clear();bool dash=false;for(unsigned char c:s){if(std::isalnum(c)){if(dash&&!out.empty())out+='-';out+=static_cast<char>(std::tolower(c));dash=false;}else dash=true;}}
    else if(id=="text-acronym") {auto v=words(s);out.clear();for(auto &w:v)if(!w.empty())out+=static_cast<char>(std::toupper((unsigned char)w[0]));}
    else if(id=="text-count-words") out=std::to_string(words(s).size());
    else if(id=="text-count-lines") out=std::to_string(s.empty()?0:1+std::count(s.begin(),s.end(),'\n')-(s.back()=='\n'));
    else if(id=="text-count-vowels") out=std::to_string(std::count_if(s.begin(),s.end(),[](unsigned char c){c=static_cast<unsigned char>(std::tolower(c));return c=='a'||c=='e'||c=='i'||c=='o'||c=='u';}));
    else if(id=="text-is-palindrome") {std::string t;for(unsigned char c:s)if(std::isalnum(c))t+=static_cast<char>(std::tolower(c));std::string r=t;std::reverse(r.begin(),r.end());out=(t==r?"true":"false");}
    else if(id=="text-unique-lines"||id=="text-sort-lines") {std::istringstream in(s);std::vector<std::string> v;std::string line;while(std::getline(in,line))v.push_back(line);if(id=="text-sort-lines")std::sort(v.begin(),v.end());else{std::set<std::string> seen;v.erase(std::remove_if(v.begin(),v.end(),[&](const auto &x){return !seen.insert(x).second;}),v.end());}out=join(v,"\n");}
    else if(id=="text-line-numbers") {std::istringstream in(s);std::string line;out.clear();size_t n=1;while(std::getline(in,line)){out+=std::to_string(n++)+": "+line+'\n';}}
    else if(id.rfind("base64-",0)==0) { static const std::string a="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; if(id=="base64-encode"){out.clear();unsigned v=0;int bits=-6;for(unsigned char c:s){v=(v<<8U)|c;bits+=8;while(bits>=0){out+=a[(v>>bits)&63U];bits-=6;}}if(bits>-6)out+=a[((v<<8U)>>(bits+8))&63U];while(out.size()%4)out+='=';}else{out.clear();unsigned v=0;int bits=-8;for(char c:s){if(c=='=')break;auto p=a.find(c);if(p==std::string::npos)continue;v=(v<<6U)|static_cast<unsigned>(p);bits+=6;if(bits>=0){out+=static_cast<char>((v>>bits)&255U);bits-=8;}}} }
    else if(id=="hex-encode"||id=="ascii-hex") {static const char h[]="0123456789abcdef";out.clear();for(unsigned char c:s){out+=h[c>>4U];out+=h[c&15U];}}
    else if(id=="hex-decode") {out.clear();for(size_t i=0;i+1<s.size();i+=2){unsigned v=0;std::istringstream in(s.substr(i,2));in>>std::hex>>v;if(in)out+=static_cast<char>(v);}}
    else if(id.find("url-")!=std::string::npos||id.find("percent-")!=std::string::npos) {out.clear();static const char h[]="0123456789ABCDEF";if(id.find("encode")!=std::string::npos){for(unsigned char c:s){if(std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')out+=static_cast<char>(c);else{out+='%';out+=h[c>>4U];out+=h[c&15U];}}}else{for(size_t i=0;i<s.size();i++){if(s[i]=='%'&&i+2<s.size()){unsigned v=0;std::istringstream in(s.substr(i+1,2));in>>std::hex>>v;if(in){out+=static_cast<char>(v);i+=2;continue;}}out+=s[i];}}}
    else if(id=="rot13"||id=="gen-rot13") {for(char &c:out){if(c>='a'&&c<='z')c=static_cast<char>('a'+(c-'a'+13)%26);else if(c>='A'&&c<='Z')c=static_cast<char>('A'+(c-'A'+13)%26);}}
    else if(id=="binary-encode"||id=="binary-decode") {out.clear();if(id=="binary-encode"){for(unsigned char c:s){for(int b=7;b>=0;b--)out+=((c>>b)&1U)?'1':'0';out+=' ';}}else{unsigned v=0,n=0;for(char c:s)if(c=='0'||c=='1'){v=(v<<1U)|static_cast<unsigned>(c-'0');if(++n==8){out+=static_cast<char>(v);v=0;n=0;}}}}
    else if(id=="csv-to-tsv"||id=="tsv-to-csv") {char from=id=="csv-to-tsv"?',':'\t',to=id=="csv-to-tsv"?'\t':',';bool quoted=false;for(char &c:out){if(c=='"')quoted=!quoted;else if(c==from&&!quoted)c=to;}}
    else if(id=="html-escape"||id=="xml-escape") {out.clear();for(char c:s){if(c=='&')out+="&amp;";else if(c=='<')out+="&lt;";else if(c=='>')out+="&gt;";else if(c=='\"')out+="&quot;";else if(c=='\'')out+="&#39;";else out+=c;}}
    else if(id=="html-unescape"||id=="xml-unescape") {out=s;for(const auto &p:std::vector<std::pair<std::string,std::string>>{{"&lt;","<"},{"&gt;",">"},{"&quot;","\""},{"&#39;","'"},{"&amp;","&"}}){size_t at=0;while((at=out.find(p.first,at))!=std::string::npos){out.replace(at,p.first.size(),p.second);at+=p.second.size();}}}
    else if(id=="json-escape"||id=="json-unescape") {out.clear();if(id=="json-escape"){out+='\"';for(unsigned char c:s){if(c=='\"'||c=='\\'){out+='\\';out+=static_cast<char>(c);}else if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else if(c=='\t')out+="\\t";else out+=static_cast<char>(c);}out+='\"';}else{bool esc=false;for(char c:s){if(esc){out+=(c=='n'?'\n':c=='r'?'\r':c=='t'?'\t':c);esc=false;}else if(c=='\\')esc=true;else if(c!='\"')out+=c;}}}
    else if(id=="morse-encode"||id=="morse-decode") {static const std::vector<std::pair<char,const char*>> table={{'A',".-"},{'B',"-..."},{'C',"-.-."},{'D',"-.."},{'E',"."},{'F',"..-."},{'G',"--."},{'H',"...."},{'I',".."},{'J',".---"},{'K',"-.-"},{'L',".-.."},{'M',"--"},{'N',"-."},{'O',"---"},{'P',".--."},{'Q',"--.-"},{'R',".-."},{'S',"..."},{'T',"-"},{'U',"..-"},{'V',"...-"},{'W',".--"},{'X',"-..-"},{'Y',"-.--"},{'Z',"--.."},{'0',"-----"},{'1',".----"},{'2',"..---"},{'3',"...--"},{'4',"....-"},{'5',"....."},{'6',"-...."},{'7',"--..."},{'8',"---.."},{'9',"----."}};out.clear();if(id=="morse-encode"){for(char c:s){if(!out.empty())out+=' ';if(std::isspace(static_cast<unsigned char>(c)))out+='/';else{char u=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));auto p=std::find_if(table.begin(),table.end(),[&](const auto &v){return v.first==u;});if(p!=table.end())out+=p->second;}}}else{std::istringstream in(s);std::string token;while(in>>token){if(token=="/"){out+=' ';continue;}auto p=std::find_if(table.begin(),table.end(),[&](const auto &v){return token==v.second;});if(p!=table.end())out+=p->first;}}}
    else if(id=="unicode-codepoints") {out.clear();for(size_t i=0;i<s.size();){unsigned char c=static_cast<unsigned char>(s[i]);unsigned cp=0;size_t n=1;if(c<0x80)cp=c;else if((c&0xe0)==0xc0&&i+1<s.size()){cp=c&0x1f;n=2;}else if((c&0xf0)==0xe0&&i+2<s.size()){cp=c&0x0f;n=3;}else if((c&0xf8)==0xf0&&i+3<s.size()){cp=c&7;n=4;}else{++i;continue;}for(size_t j=1;j<n;j++)cp=(cp<<6U)|(static_cast<unsigned char>(s[i+j])&0x3fU);std::ostringstream code;code<<"U+"<<std::uppercase<<std::hex<<cp;out+=code.str()+" ";i+=n;}if(!out.empty())out.pop_back();}
    else if(id.rfind("num-",0)==0||id.rfind("science-",0)==0) {std::istringstream in(s);double x=0,y=0;in>>x;std::string r;if(id.find("celsius")!=std::string::npos)r=std::to_string(x*9/5+32);else if(id.find("fahrenheit")!=std::string::npos)r=std::to_string((x-32)*5/9);else if(id.find("kelvin")!=std::string::npos)r=std::to_string(x-273.15);else if(id.find("factorial")!=std::string::npos){unsigned long long n=x<0?0:static_cast<unsigned long long>(x),v=1;for(unsigned long long i=2;i<=n&&i<21;i++)v*=i;r=std::to_string(v);}else if(id.find("fibonacci")!=std::string::npos){unsigned long long a=0,b=1,n=x<0?0:static_cast<unsigned long long>(x);for(unsigned long long i=0;i<n&&i<94;i++){auto t=a+b;a=b;b=t;}r=std::to_string(a);}else if(id.find("prime")!=std::string::npos){long long n=static_cast<long long>(x);bool p=n>=2;for(long long i=2;p&&i<=n/i;i++)if(n%i==0)p=false;r=p?"true":"false";}else if(id.find("sqrt")!=std::string::npos)r=std::to_string(std::sqrt(std::max(0.0,x)));else if(id.find("cube")!=std::string::npos)r=std::to_string(std::cbrt(x));else if(id.find("circle")!=std::string::npos)r=std::to_string(3.141592653589793*x*x);else if(id.find("percent")!=std::string::npos){in>>y;r=std::to_string(x*y/100.0);}else r=std::to_string(x);out=r;}
    else if(id.rfind("gen-",0)==0) {if(id.find("uuid")!=std::string::npos||id.find("id")!=std::string::npos)out="00000000-0000-4000-8000-000000000001";else if(id.find("password")!=std::string::npos||id.find("token")!=std::string::npos)out="Kaya-7mQ4-vP9x-2dR8";else if(id.find("color")!=std::string::npos)out="#4F86F7";else if(id.find("lorem")!=std::string::npos)out="Lorem ipsum dolor sit amet.";else out="#!/usr/bin/env sh\n# Generated by Kaya\n";}
    else if(id.rfind("dev-",0)==0||id.rfind("data-",0)==0) {if(id.find("count")!=std::string::npos)out=std::to_string(s.size());else if(id.find("quote")!=std::string::npos)out="'"+s+"'";else if(id.find("unique")!=std::string::npos||id.find("dedupe")!=std::string::npos){std::istringstream in(s);std::set<std::string> seen;std::string line;out.clear();while(std::getline(in,line))if(seen.insert(line).second)out+=line+'\n';}}
    else {std::cerr<<"Unknown tool: "<<id<<'\n';return 2;}
    std::cout<<out; if(out.empty()||out.back()!='\n')std::cout<<'\n'; return 0;
}
}
