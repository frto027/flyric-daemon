#ifndef FLYRICWINDOWTHREAD_H
#define FLYRICWINDOWTHREAD_H

#include <QObject>
#include <QThread>
#include <flyricconfigmanager.h>
class FlyricWindowThread : public QThread
{
    Q_OBJECT
    Q_PROPERTY(bool isBackgroundTransparent MEMBER isTransparent NOTIFY isTransparentChanged)
    Q_PROPERTY(bool isTop MEMBER isTop NOTIFY isTopChanged)
    Q_PROPERTY(bool isResizeable MEMBER isResizeable NOTIFY isResizeableChanged)
public:

protected:
    void run();
signals:
    void windowExit(int code);
    void isTransparentChanged();
    void isResizeableChanged();
    void isTopChanged();

public slots:
    void createAndShow(FlyricConfigManager * manager);

    void exitWindow();
private:
    FlyricConfigManager * config = nullptr;

    bool isTransparent = false;
    bool isResizeable = false;
    bool noBreak = true;
    bool isTop = true;

    QJsonObject windConf;
};

#endif // FLYRICWINDOWTHREAD_H
