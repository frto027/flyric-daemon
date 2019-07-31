#include "flyricwindowthread.h"
extern "C"{
#include "glad/glad.h"
#include "GLFW/glfw3.h"
}

#include <ft2build.h>
#include FT_FREETYPE_H

#include <QDebug>

#define KEY_TRANS "isTrans"
#define KEY_L "PosL"
#define KEY_T "PosT"
#define KEY_W "PosW"
#define KEY_H "PosH"

void FlyricWindowThread::createAndShow(FlyricConfigManager *manager){
    config = manager;
    config->getWindowConfigure(windConf);

    start();
}

void FlyricWindowThread::exitWindow(){
    noBreak = false;
}
void FlyricWindowThread::run(){
    FT_Library ftlib;
    if(FT_Init_FreeType(&ftlib)){
        qDebug()<<"Freetype init error";
    }else{
        qDebug()<<"Freetype init success";
    }

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

    GLFWwindow* window;

    if (!glfwInit()){
        emit windowExit(EXIT_FAILURE);
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_RESIZABLE,isResizeable ? GLFW_TRUE: GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED,isResizeable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER,isTransparent ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING,isTop ? GLFW_TRUE : GLFW_FALSE);

    window = glfwCreateWindow(
                windConf.contains(KEY_W)?windConf[KEY_W].toInt():640,
                windConf.contains(KEY_H)?windConf[KEY_H].toInt():480,
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

    bool checkIgnoreMouse = true;

    while (noBreak && !glfwWindowShouldClose(window))
    {
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
                //glfwSetWindowAttrib(window,)
            }
        }

        glViewport(0, 0, 640, 480);
        glClear(GL_COLOR_BUFFER_BIT);


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

    FT_Done_FreeType(ftlib);

    /*save configure*/
    config->setWindowConfigure(windConf);
    emit windowExit(EXIT_SUCCESS);
}
