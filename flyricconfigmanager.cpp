#include "flyricconfigmanager.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>

#include <QFile>
#include <QJsonDocument>

FlyricConfigManager::FlyricConfigManager(QObject *parent) : QObject(parent)
{
    load();
}

QString FlyricConfigManager::getDefaultFontPath(){
    if(savedObject.contains("defaultFont"))
        return savedObject["defaultFont"].toString();

    static const char *defaultPath[] = {
        "C:/Windows/Fonts/msyh.ttc",//微软雅黑
        "C:/Windows/Fonts/Deng.ttf",//等线
        "C:/Windows/Fonts/arial.ttf",//arial
        //TODO:add linux fonts here
        nullptr
    };

    for(int i=0;defaultPath[i] !=nullptr;i++){
        if(QFileInfo(defaultPath[i]).exists()){
            return defaultPath[i];
        }
    }
    //TODO:dynamicaly select a fonts
    return "";
}
QStringList FlyricConfigManager::getDefaultFontFolders(){
    QStringList ret;
    if(savedObject.contains("fontFolder")){
        QJsonArray arr = savedObject["fontFolder"].toArray();
        foreach(const QJsonValue &obj,arr){
            ret.push_back(obj.toString());
        }
    }else{
        ret.append("C:/Windows/Fonts");
    }

    return ret;
}
bool FlyricConfigManager::setDefaultFont(QString defaultFont){
    savedObject["defaultFont"] = defaultFont;
    return true;
}
bool FlyricConfigManager::setFontFolder(QStringList fonts){
    savedObject["fontFolder"] = QJsonArray::fromStringList(fonts);
    return true;
}
bool FlyricConfigManager::setUdpPort(int port){
    savedObject["udpPort"]=port;
    return true;
}
int FlyricConfigManager::getUdpPort(){
    if(savedObject.contains("udpPort")){
        return savedObject["udpPort"].toInt();
    }
    return 9588;
}

void FlyricConfigManager::setWindowConfigure(const QJsonObject &obj){
    savedObject["windConf"]=obj;
    save();
}
void FlyricConfigManager::getWindowConfigure(QJsonObject &obj){
    if(savedObject.contains("windConf")){
        obj = savedObject["windConf"].toObject();
    }else{
        obj = QJsonObject();
    }
}

QString FlyricConfigManager::getFrcFolder(){
    if(savedObject.contains("frcFolder")){
        return savedObject["frcFolder"].toString();
    }
    return QFileInfo(".").absolutePath();
}
void FlyricConfigManager::setFrcFolder(QString frcFolder){
    savedObject["frcFolder"]=frcFolder;
}

bool FlyricConfigManager::save(){
    QFile saveFile(getConfigFilePath());
    if(!saveFile.open(QIODevice::WriteOnly)){
        return false;
    }

    QJsonDocument doc(savedObject);
    saveFile.write(doc.toJson());
    return true;
}
bool FlyricConfigManager::load(){
    QFile saveFile(getConfigFilePath());

    if(!saveFile.open(QIODevice::ReadOnly)){
        //default config
        return true;
    }

    savedObject = QJsonDocument::fromJson(saveFile.readAll()).object();
    return true;
}
