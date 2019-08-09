#include "flyricwindowthread.h"
extern "C"{
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "flyric_rendergl.h"
}

#include <ft2build.h>
#include FT_FREETYPE_H



#include <QFile>
#include <QDateTime>
#include <QNetworkDatagram>
#include <QtEndian>
#include <QFileInfo>
#include <QDirIterator>
#include <QMap>
#include <QDebug>

#define KEY_TRANS "isTrans"
#define KEY_L "PosL"
#define KEY_T "PosT"
#define KEY_W "PosW"
#define KEY_H "PosH"

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480

#define FONT_SIZE 30

static char predefined_loading_lyric[] = "[curve]\n"
                                         "mm(x):x*x*x*x*x*x\n"
                                         "midfast(x):6*(x*x/2-x*x*x/3)\n"
                                         "\n"
                                         "[anim]\n"
                                         "Name,Property,Func,During,Offset,Hard\n"
                                         "Fade,ColorR,mm((x-0.5)*2),[Duration],0,F\n"
                                         "Swing,TransX,sin(midfast(x)*2*3.1415926 * 20) * 0.1,[Duration],0,F\n"
                                         "Rotate,RotateZ,midfast(x)*2*3.1415926 * 20,[Duration],0,F\n"
                                         "Shake,RotateZ,sin(midfast(x)*2*3.1415926 * 20) * 0.1,[Duration],0,F\n"
                                         "[flyc]\n"
                                         "Type,StartTime,Text,Duration,ColorR,ColorG,ColorB,ColorA,Anim,RotateZ,AnchorX,AnchorY,SelfAnchorX,SelfAnchorY,TransX,TransY,RotateZ\n"
                                         "line,0,N,1000,1,0,1,0,Fade,0,0.5,0.5,0.5,0.5\n"
                                         "word,0,n(*≧▽≦*)n ,4000,1,0,1,0,Fade|Swing|Shake\n"
                                        ",,---,,,,,,Rotate\n"
                                         ",200,fly,4000,1,0,1,0,Fade\n"
                                         ",>>200,ric\n,, -\n,,0.0.0\n,,-\n,,preview\n";
static FRPFile * predefined_loading_lyric_file;
#define PREDEFINED_LOAD_LYRIC_TIME 10000

void FlyricWindowThread::createAndShow(FlyricConfigManager *manager){
    config = manager;
    config->getWindowConfigure(windConf);

    start();
}

void FlyricWindowThread::exitWindow(){
    noBreak = false;
}

qint64 FlyricWindowThread::getTime(){
    return QDateTime::currentMSecsSinceEpoch();
}

void FlyricWindowThread::play(qint64 play_begin_time){
    this->play_begin_time = play_begin_time;
    play_is_paused = 0;
}
void FlyricWindowThread::play_continue(){
    if(play_is_paused){
        play(getTime() - paused_time);
    }
}
void FlyricWindowThread::pause(qint64 paused_time){
    this->paused_time = paused_time;
    play_is_paused = 1;
}
void FlyricWindowThread::pause_now(){
    if(!play_is_paused)
        pause(getTime() - play_begin_time);
}

void FlyricWindowThread::switch_lyric(QString lyric_name){
    QString * nname = new QString(lyric_name);
    nname = this->switch_lyric_name.fetchAndStoreRelaxed(nname);
    if(nname != nullptr){
        delete nname;
    }
}

void glfwErrCb(int err,const char * msg){
    qDebug()<<"GLFW Error["<<err<<"]:"<<msg;
}

void FlyricWindowThread::run(){
    if(config){
        auto d = qDebug()<<"Configure ok:\n";
        d<<"Default font path:"<<config->getDefaultFontPath()<<"\n";
        d<<"Folders:"<<config->getDefaultFontFolders()<<"\n";
        d<<"Port:"<<config->getUdpPort();

        /* configure transparent */
        if(!windConf.contains("isTrans"))
            windConf[KEY_TRANS]=false;
        isTransparent = windConf[KEY_TRANS].toBool();
        emit isTransparentChanged();
    }else{
        qDebug()<<"No configure";
    }

    bool isTransparent = this->isTransparent;
    bool isResizeable = this->isResizeable;
    bool isTop = this->isTop;

    bool checkIgnoreMouse = true;
    qint64 current_lyric_time;

    play_begin_time = getTime();
    FRPFile * lyric_file = nullptr;
    bool lyric_is_loaded = false;

    QDir frc_dir(config->getFrcFolder());
    QString current_lyric_name("");

    GLFWwindow* window;

    if (!glfwInit()){
        emit windowExit(EXIT_FAILURE);
        return;
    }

    glfwSetErrorCallback(&glfwErrCb);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    glfwWindowHint(GLFW_RESIZABLE,isResizeable ? GLFW_TRUE: GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED,isResizeable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER,isTransparent ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING,isTop ? GLFW_TRUE : GLFW_FALSE);


    window = glfwCreateWindow(
                windConf.contains(KEY_W)?windConf[KEY_W].toInt():DEFAULT_WINDOW_WIDTH,
                windConf.contains(KEY_H)?windConf[KEY_H].toInt():DEFAULT_WINDOW_HEIGHT,
                "WIND",nullptr,nullptr);
    if (!window)
    {
        glfwTerminate();
        emit windowExit(EXIT_FAILURE);
        return;
    }
    glfwSetWindowPos(
                window,
                windConf.contains(KEY_L)?windConf[KEY_L].toInt():40,
                windConf.contains(KEY_T)?windConf[KEY_T].toInt():40);

    glfwMakeContextCurrent(window);
    gladLoadGL();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    /* Init library */
    FT_Library ftlib;
    if(FT_Init_FreeType(&ftlib)){
        qDebug()<<"Freetype init error";
    }else{
        qDebug()<<"Freetype init success";
    }
    frpstartup();

    frg_startup(ftlib,config->getDefaultFontPath().toUtf8(),0);


    //enum fonts
    const char * supported_font_format[] = {"ttf","ttc","fnt"};
    foreach(const QString & dir,config->getDefaultFontFolders()){
        qDebug()<<"Reading font in dir:"<<dir;
        QDirIterator dir_it(dir,QDir::Filter::Files,QDirIterator::Subdirectories);
        QStringList fmaplist;

        while(dir_it.hasNext()){
            QFileInfo file(dir_it.next());
            if(!file.isFile())
                continue;

            if(file.suffix().toLower() == "fmap"){
                fmaplist<<file.absoluteFilePath();
                continue;
            }

            size_t i;
            for(i=0;i<sizeof(supported_font_format)/sizeof(*supported_font_format);i++){
                if(file.suffix().toLower() == supported_font_format[i])
                    break;
            }
            if(i == sizeof(supported_font_format)/sizeof(*supported_font_format)){
                //qDebug()<<"Unsupport format "<<file.suffix();
                continue;
            }

            qDebug()<<" find font file:"<<file.absoluteFilePath() <<" called:"<<file.baseName();
            frg_add_font_file(ftlib,reinterpret_cast<frp_uint8 *>(file.baseName().toUtf8().data()),file.absoluteFilePath().toUtf8(),0);
        }

        //dir over
        fmaplist.sort();
        foreach(const QString & fmapfile,fmaplist){
            qDebug()<<" parse fmap file:"<<fmapfile;
            QFile fmap(fmapfile);
            if(!fmap.open(QIODevice::ReadOnly|QIODevice::Text))
                continue;
            QTextStream in(&fmap);
            QString line;
            while(!(line = in.readLine()).isNull()){
                auto list = line.split("=>");
                if(list.length() > 1){
                    qDebug()<<"alias (known)"<<list[0]<<" to (new)"<<list[1];
                    frg_add_font_alias(
                                reinterpret_cast<frp_uint8 *>(list[1].toUtf8().data()),
                            reinterpret_cast<frp_uint8 *>(list[0].toUtf8().data()));
                }
            }

        }
    }

    frg_fontsize_set(FONT_SIZE);
    frg_screensize_set(frp_size(windConf.contains(KEY_W)?windConf[KEY_W].toInt():DEFAULT_WINDOW_WIDTH),
                       frp_size(windConf.contains(KEY_H)?windConf[KEY_H].toInt():DEFAULT_WINDOW_HEIGHT));

    glfwSetWindowSizeCallback(window,[](GLFWwindow *,int w,int h){
        frg_screensize_set(frp_size(w),frp_size(h));
    });

    predefined_loading_lyric_file = frpopen(reinterpret_cast<unsigned char *>(predefined_loading_lyric),sizeof(predefined_loading_lyric),1);
    frg_loadlyric(predefined_loading_lyric_file);

    /* Debug only */
    //load a temp lyric
#if 0
    {
        QFile file("H:/test.frc");
        if(file.open(QIODevice::ReadOnly)){
            auto bts = file.readAll();

            lyric_file = frpopen(reinterpret_cast<unsigned char *>(bts.data()),frp_size(file.size()),0);
            frg_loadlyric(lyric_file);
            lyric_is_loaded = true;
            qDebug()<<"Debug:Load lyric success";
        }else{
            qDebug()<<"Debug:Load failed";
        }
    }
#endif

    double last_time = glfwGetTime();
    while (noBreak && !glfwWindowShouldClose(window))
    {
        double remain = glfwGetTime() - last_time;
        if(remain <  1./60){
            msleep(static_cast<unsigned long>(1./60 - remain));
        }
        last_time += 1./60;

        {
            QString * lyric_name = this->switch_lyric_name.fetchAndStoreRelaxed(nullptr);
            if(lyric_name != nullptr){
                //TODO:switch to new lyric
                qDebug()<<"Switch lyric:"<<*lyric_name;
                if(current_lyric_name != *lyric_name){
                    frg_unloadlyrc();
                    if(lyric_file)
                        frpdestroy(lyric_file);
                    lyric_file = nullptr;

                    try {
                        QFileInfo finfo(frc_dir,(*lyric_name)+".frc");
                        qDebug()<<(*lyric_name)+".frc";
                        if(!finfo.absoluteDir().absolutePath().startsWith(frc_dir.absolutePath()) || !finfo.exists())
                            throw 1;//1 means file not exist
                        QFile file(finfo.absoluteFilePath());
                        if(!file.open(QIODevice::ReadOnly))
                            throw 2;//2 means file open failed
                        QByteArray arr(file.readAll());
                        lyric_file = frpopen(reinterpret_cast<unsigned char *>(arr.data()),frp_size(arr.length()),0);
                        if(!lyric_file)
                            lyric_file = nullptr;
                        if(lyric_file == nullptr)
                            throw 3;//3 means parse error
                        frg_loadlyric(lyric_file);
                        lyric_is_loaded = true;
                        current_lyric_name = *lyric_name;
                    } catch (int) {
                        //load lyric failed
                        frg_loadlyric(predefined_loading_lyric_file);
                        lyric_is_loaded = false;
                        current_lyric_name = "";
                    }
                }
                delete lyric_name;
            }
        }

        if(isResizeable != this->isResizeable){
            isResizeable = this->isResizeable;
            glfwSetWindowAttrib(window,GLFW_RESIZABLE,isResizeable ? 1 : 0);
            glfwSetWindowAttrib(window,GLFW_DECORATED,isResizeable ? GLFW_TRUE : GLFW_FALSE);
            checkIgnoreMouse = true;
        }
        if(isTransparent != this->isTransparent){
            isTransparent = this->isTransparent;
            glfwSetWindowAttrib(window,GLFW_TRANSPARENT_FRAMEBUFFER,isTransparent ? GLFW_TRUE : GLFW_FALSE);
            checkIgnoreMouse = true;
        }
        if(isTop != this->isTop){
            isTop = this->isTop;
            glfwSetWindowAttrib(window,GLFW_FLOATING,isTop ? GLFW_TRUE : GLFW_FALSE);
            checkIgnoreMouse = true;
        }

        if(checkIgnoreMouse){
            checkIgnoreMouse = false;
            if(isTop && isTransparent && !isResizeable){
                glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_IGNORE);
            }else{
                glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
            }
        }
        {
            int w,h;
            glfwGetWindowSize(window,&w,&h);
            glViewport(0, 0, w,h);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        {
            if(lyric_is_loaded){
                if(play_is_paused){
                    current_lyric_time = paused_time;
                }else{
                    current_lyric_time = getTime() - play_begin_time;
                }
                for(auto node = frp_play_getline_by_time(lyric_file,current_lyric_time);node;node=node->next){
                    frg_renderline(node->line,current_lyric_time);
                }
            }else{
                qint64 tim = getTime() - play_begin_time;
                tim %= PREDEFINED_LOAD_LYRIC_TIME;

                for(auto node = frp_play_getline_by_time(predefined_loading_lyric_file,tim);node;node=node->next){
                    frg_renderline(node->line,tim);
                }
            }


        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    /* save window position */
    windConf[KEY_TRANS] = this->isTransparent;
    {
        int l,t,w,h;
        glfwGetWindowPos(window,&l,&t);
        glfwGetWindowSize(window,&w,&h);
        windConf[KEY_L]=l;
        windConf[KEY_T]=t;
        windConf[KEY_W]=w;
        windConf[KEY_H]=h;
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    frg_shutdown();

    frpshutdown();

    FT_Done_FreeType(ftlib);

    /*save configure*/
    config->setWindowConfigure(windConf);
    emit windowExit(EXIT_SUCCESS);
}

/* udp server */

void FlyricWindowThread::startUdpServer(){
    if(!config || config == nullptr){
        qDebug()<<"Error UDP init:no config";
        return;
    }
    int port = config->getUdpPort();
    qDebug()<<"Listening udp at localhost:"<<port;
    if(udp_socket != nullptr){
        delete udp_socket;
    }
    udp_socket = new QUdpSocket(this);
    udp_socket->bind(QHostAddress::LocalHost,quint16(port));
    connect(udp_socket,SIGNAL(readyRead()),this,SLOT(datagramReceived()));

}

void FlyricWindowThread::datagramReceived(){
    while(udp_socket->hasPendingDatagrams()){
        auto datagram = udp_socket->receiveDatagram();
        parseDatagram(datagram.data());
    }
}
void FlyricWindowThread::parseDatagram(const QByteArray &data){
    /* skip id */
    const quint32 skip_range = 0x7FFFFFFF;
    static quint32 skip_data_from = 0 - skip_range;
    static quint32 skip_data_to = 0;
    const char * cdata = data.data();
    if(data.length() < 8){
        //invalid data
        return;
    }
    quint32 id = qFromBigEndian<quint32>(cdata);
    if(id){
        if(skip_data_from < skip_data_to){
            if(skip_data_from <= id && id <= skip_data_to){
                return;
            }
        }else{
            if(skip_data_from <= id || id <= skip_data_to){
                return;
            }
        }
    }
    skip_data_to = id;
    skip_data_from = id - skip_range;

    quint32 type = qFromBigEndian<quint32>(cdata + 4);
    switch(type){
    case UDP_DATA_TYPE_LOADLYRIC:
    {
        int actsize = 0;
        while(8 + actsize < data.size() && cdata[8 + actsize]){
            actsize++;
        }

        QString * lyric_name = new QString(QString::fromUtf8(cdata + 8,actsize));
        lyric_name = this->switch_lyric_name.fetchAndStoreRelaxed(lyric_name);
        if(lyric_name){
            //说明有一个歌词来不及换就又上了另一个歌词，以这个为准
            delete lyric_name;
        }
    }
        break;
    case UDP_DATA_TYPE_PAUSE_TIME:
    {
        if(data.length() < 16)
            return;
        qint64 time = qFromBigEndian<qint64>(cdata + 8);
        this->pause(time);
    }
        break;
    case UDP_DATA_TYPE_PLAY_TIME:
    {
        if(data.length() < 16)
            return;
        qint64 time = qFromBigEndian<qint64>(cdata + 8);
        this->play(time);
    }
        break;
    case UDP_DATA_TYPE_PLAY_NOW:
        this->play_continue();
        break;
    case UDP_DATA_TYPE_PAUSE_NOW:
        this->pause_now();
        break;
    }
}
