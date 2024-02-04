//
// Created by Jack on 2023/9/24.
//
#include "protocol.h"

packet analyzePacket(const std::string &str) {
    unsigned short type = 0, id = 0;
    std::string text;

    int i = 0;
    for(; i < 3; i++) type = type * 2 + (str[i] - '0');
    for(; str[i] != '|'; i++) id = id * 2 + (str[i] - '0');

    return {type, id, str.substr(i+1, str.length())};
}

std::string pac2str( const packet &p )
{
    std::string stype, sid;
    unsigned short type = p.getType(), id = p.getId();

    int i = 0;
    for( ; i < 3; type/=2, i++ ) stype = std::to_string( type % 2 ) + stype;
//    for( ; ; id/=2, i++ ) sid = std::to_string( id % 2 ) + sid;

    if(id == 0) sid = "0|";
    else{
	sid = "";
        while(id != 0){
            if(id % 2 == 1) sid = "1" + sid;
            else sid = "0" + sid;
            id /= 2;
        }
        sid += "|";
    }

    return stype + sid + p.getText();
}
