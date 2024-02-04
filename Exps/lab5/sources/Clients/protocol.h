//
// Created by Jack on 2023/9/24.

//
//000:连接
//001:断开连接
//010:获取时间
//011:获取名字
//100:获取所有用户信息
//101:发消息
//

#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

#include <iostream>
#include <string>
#include <utility>

class packet{
private:
    unsigned short type;
    unsigned short id;
    std::string text;

public:
    packet(unsigned short type, unsigned short id = 0, std::string text = "") : type(type), id(id), text(std::move(text)){};
    packet(const packet &p){
        type = p.type;
        id = p.id;
        text = p.text;
    }

    [[nodiscard]] unsigned short getType() const{return type;};
    [[nodiscard]] unsigned short getId() const{return id;};
    std::string getText() const {return text;};

    void setType(unsigned short TYPE){type = TYPE;}
    void setId(unsigned short ID){id = ID;};
    void setText(std::string TEXT){text = std::move(TEXT);};


//    std::string format();

    //debug用
    void printInfo(){
        std::cout << "type: " << type << std::endl;
        std::cout << "id: " << id << std::endl;
        std::cout << "text: " << text << std::endl;
    };
};

    packet analyzePacket(const std::string &p); //从字符串构造包（仅适用类别、ID完整的包）
    std::string pac2str(const packet &p); //从包构建字符串

#endif //CLIENT_PROTOCOL_H
