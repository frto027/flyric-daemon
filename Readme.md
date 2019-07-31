# 这是什么
这是一个正在开发中的flyric歌词显示软件，它通过接收UDP包来调整歌词显示的进度和加载歌词文件

# IDE及开发环境
如果想要编译或运行这个软件，我目前使用的开发环境是
```
Windows10 1903
Qt 5.12.3 MinGW 7.3.0 64bit
Intel HD 4600集成显卡或NVIDIA GTX 860M显卡
OpenGL version string: 4.3.0 - Build 20.19.15.4531
```
```
程序需要OpenGL 4.2或以上版本
除Qt的库之外，只有glfw是以库的形式依赖的，其余库直接以源码形式引用，并配置在qt工程文件中,glfw暂时只支持Windows 64位
```
# 相关项目
- [QT5](https://www.qt.io/)(配置界面等除OpenGL外的相关UI)
- [Glad](https://github.com/dav1dde/glad-web)(OpenGL函数)
- [GLFW](https://www.glfw.org/)(提供OpenGL环境)
- [flyric_rendergl](https://github.com/frto027/flyric_rendergl)(歌词解码/绘制)
- [FreeType(flyric_rendergl依赖)](https://www.freetype.org/)(字体处理)
