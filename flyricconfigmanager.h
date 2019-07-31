#ifndef FLYRICFONTMANAGER_H
#define FLYRICFONTMANAGER_H

#include <QObject>
#include <QJsonObject>

class FlyricConfigManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString confPath READ getConfigFilePath CONSTANT)
public:
    explicit FlyricConfigManager(QObject *parent = nullptr);

    Q_INVOKABLE QString getDefaultFontPath();
    Q_INVOKABLE QStringList getDefaultFontFolders();
    Q_INVOKABLE int getUdpPort();

    Q_INVOKABLE bool setDefaultFont(QString defaultFont);
    Q_INVOKABLE bool setFontFolder(QStringList fonts);
    Q_INVOKABLE bool setUdpPort(int port);

    Q_INVOKABLE bool save();
    Q_INVOKABLE bool load();

    void getWindowConfigure(QJsonObject & obj);
    void setWindowConfigure(const QJsonObject & obj);

signals:

public slots:

private:
    QJsonObject savedObject;
    static QString getConfigFilePath(){
        return "D:/test.json";
    }
};

#endif // FLYRICFONTMANAGER_H
