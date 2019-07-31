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

#include <QDebug>

#define KEY_TRANS "isTrans"
#define KEY_L "PosL"
#define KEY_T "PosT"
#define KEY_W "PosW"
#define KEY_H "PosH"

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480

#define FONT_SIZE 60

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

    GLFWwindow* window;

    if (!glfwInit()){
        emit windowExit(EXIT_FAILURE);
        return;
    }
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

    frg_fontsize_set(FONT_SIZE);
    frg_screensize_set(frp_size(windConf.contains(KEY_W)?windConf[KEY_W].toInt():DEFAULT_WINDOW_WIDTH),
                                       frp_size(windConf.contains(KEY_H)?windConf[KEY_H].toInt():DEFAULT_WINDOW_HEIGHT));

    glfwSetWindowSizeCallback(window,[](GLFWwindow *,int w,int h){
       frg_screensize_set(frp_size(w),frp_size(h));
    });

    /* Debug only */
    //load a temp lyric
    {
        QFile file("C:/Users/q6027/Desktop/lrc.txt");
        if(file.open(QIODevice::ReadOnly)){
            auto bts = file.readAll();
            lyric_file = frpopen((unsigned char *)bts.data(),file.size(),0);
            frg_loadlyric(lyric_file);
            lyric_is_loaded = true;
            qDebug()<<"Debug:Load lyric success";
        }else{
            qDebug()<<"Debug:Load failed";
        }
    }
    while (noBreak && !glfwWindowShouldClose(window))
    {
        {
            QString * lyric_name = this->switch_lyric_name.fetchAndStoreRelaxed(nullptr);
            if(lyric_name != nullptr){
                //TODO:switch to new lyric

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
                //TODO ???
                //GLFW目前是否支持相关特性？ https://github.com/glfw/glfw/issues/1236
                //需要修改GLFW源代码
            }
        }
        {
            int w,h;
            glfwGetWindowSize(window,&w,&h);
            glViewport(0, 0, w,h);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        if(lyric_is_loaded){
            if(play_is_paused){
                current_lyric_time = paused_time;
            }else{
                current_lyric_time = getTime() - play_begin_time;
            }

            for(auto node = frp_play_getline_by_time(lyric_file,current_lyric_time);node;node=node->next){
                frg_renderline(node->line,current_lyric_time);
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
