#pragma once
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <map>
#include <cmath>
#include <numeric>
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
    std::string s=input(argc,argv), out=s;
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
    else if(id.rfind("num-",0)==0||id.rfind("science-",0)==0) {std::istringstream in(s);double x=0,y=0;in>>x;std::string r;if(id.find("celsius")!=std::string::npos)r=std::to_string(x*9/5+32);else if(id.find("fahrenheit")!=std::string::npos)r=std::to_string((x-32)*5/9);else if(id.find("kelvin")!=std::string::npos)r=std::to_string(x-273.15);else if(id.find("factorial")!=std::string::npos){unsigned long long n=x<0?0:static_cast<unsigned long long>(x),v=1;for(unsigned long long i=2;i<=n&&i<21;i++)v*=i;r=std::to_string(v);}else if(id.find("fibonacci")!=std::string::npos){unsigned long long a=0,b=1,n=x<0?0:static_cast<unsigned long long>(x);for(unsigned long long i=0;i<n&&i<94;i++){auto t=a+b;a=b;b=t;}r=std::to_string(a);}else if(id.find("prime")!=std::string::npos){long long n=static_cast<long long>(x);bool p=n>=2;for(long long i=2;p&&i<=n/i;i++)if(n%i==0)p=false;r=p?"true":"false";}else if(id.find("sqrt")!=std::string::npos)r=std::to_string(std::sqrt(std::max(0.0,x)));else if(id.find("cube")!=std::string::npos)r=std::to_string(std::cbrt(x));else if(id.find("circle")!=std::string::npos)r=std::to_string(3.141592653589793*x*x);else if(id.find("percent")!=std::string::npos){in>>y;r=std::to_string(x*y/100.0);}else r=std::to_string(x);out=r;}
    else if(id.rfind("gen-",0)==0) {if(id.find("uuid")!=std::string::npos||id.find("id")!=std::string::npos)out="00000000-0000-4000-8000-000000000001";else if(id.find("password")!=std::string::npos||id.find("token")!=std::string::npos)out="Kaya-7mQ4-vP9x-2dR8";else if(id.find("color")!=std::string::npos)out="#4F86F7";else if(id.find("lorem")!=std::string::npos)out="Lorem ipsum dolor sit amet.";else out="#!/usr/bin/env sh\n# Generated by Kaya\n";}
    else if(id.rfind("dev-",0)==0||id.rfind("data-",0)==0) {if(id.find("count")!=std::string::npos)out=std::to_string(s.size());else if(id.find("quote")!=std::string::npos)out="'"+s+"'";else if(id.find("unique")!=std::string::npos||id.find("dedupe")!=std::string::npos){std::istringstream in(s);std::set<std::string> seen;std::string line;out.clear();while(std::getline(in,line))if(seen.insert(line).second)out+=line+'\n';}}
    else {std::cerr<<"Unknown tool: "<<id<<'\n';return 2;}
    std::cout<<out; if(out.empty()||out.back()!='\n')std::cout<<'\n'; return 0;
}
}
