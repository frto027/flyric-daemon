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

    void play(qint64 play_begin_time);
    void play_continue();
    void pause(qint64 paused_time);
    void pause_now();

    void switch_lyric(QString lyric_name);

    void exitWindow();
private:
    FlyricConfigManager * config = nullptr;

    bool isTransparent = false;
    bool isResizeable = false;
    bool noBreak = true;
    bool isTop = true;

    QJsonObject windConf;
    QAtomicInt play_is_paused = 0;
    QAtomicInteger<qint64> play_begin_time;//歌曲播放开始时刻的绝对时间
    QAtomicInteger<qint64> paused_time;//这个是暂停时歌曲的播放进度

    QAtomicPointer<QString> switch_lyric_name;
protected:
    ///
    /// \brief getTime 用来做时间同步，返回值需要是一个绝对时间(比如系统时钟)，并非歌曲播放的时间
    /// \return time ms
    ///
    virtual qint64 getTime();

};

#endif // FLYRICWINDOWTHREAD_H
