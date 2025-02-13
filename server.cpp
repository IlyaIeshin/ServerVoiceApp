#include "server.h"
#include <QDebug>

Server::Server(QObject *parent) : QObject(parent) {
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

void Server::startServer(quint16 port) {
    if (server->listen(QHostAddress::Any, port)) {
        qDebug() << "Сервер запущен на порту" << port;
    } else {
        qDebug() << "Ошибка запуска сервера!";
    }
}

void Server::onNewConnection() {
    QTcpSocket *clientSocket = server->nextPendingConnection();
    clients.append(clientSocket);
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    qDebug() << "Новый клиент подключился!";
}

void Server::onReadyRead() {
    for (QTcpSocket *client : clients) {
        if (client->bytesAvailable()) {
            QByteArray data = client->readAll();
            qDebug() << "Получены данные: " << data;
        }
    }
}
