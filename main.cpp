#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ScoresServer.h"
#include "ScoresClient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QString host = "127.0.0.1";
    int port = 12345;

    // Start the server
    ScoresServer server(port);

    // Create the client
    ScoresClient client(host, port);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Client", &client);
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );
    engine.load(url);

    return app.exec();
}
