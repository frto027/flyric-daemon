#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>

#include "flyricconfigmanager.h"

#include "flyricwindowthread.h"

int main(int argc, char *argv[])
{

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    qmlRegisterType<FlyricConfigManager>("top.frto027.flyric",1,0,"FlyricConfigManager");
    qmlRegisterType<FlyricWindowThread>("top.frto027.flyric",1,0,"FlyricWindowThread");

    app.setWindowIcon(QIcon(":/icon.png"));

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
