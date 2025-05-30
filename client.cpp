//client.cpp
#include "client.h"

Client::Client(const QUrl &serverUrl, bool debug, QObject *parent) :
    QObject(parent), m_serverUrl(serverUrl), m_debug(debug)
{
    if (m_debug)
        qDebug() << "\nПодключение к серверу:" << m_serverUrl.toString();

    if (!m_serverUrl.isValid() || m_serverUrl.scheme() != "ws") {
        qWarning() << "Неверный URL сервера:" << m_serverUrl.toString();
        return;
    }

    connect(&m_webSocket, &QWebSocket::connected, this, &Client::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &Client::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &Client::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &Client::onErrorOccurred);

    connect(&m_reconnectTimer, &QTimer::timeout, this, &Client::tryReconnect);
    m_reconnectTimer.setInterval(WAITING_TIME);
    m_reconnectTimer.setSingleShot(false);

    m_webSocket.open(m_serverUrl);
}

Client::~Client()
{
    if (m_webSocket.state() == QAbstractSocket::ConnectedState)
        m_webSocket.close();
}

void Client::onConnected()
{
    if (m_debug)
    {
        qDebug() << "Успешное подключение к серверу!";
        qDebug() << "Клиентский порт:" << m_webSocket.localPort();
    }

    emit connected();

    if (m_debug)
        m_webSocket.sendTextMessage("Hello, EchoServer!");
}

void Client::tryReconnect()
{
    m_reconnectTimer.stop();
    if (m_webSocket.state() != QAbstractSocket::ConnectedState)
    {
        if (MAX_RECONNECT_ATTEMPTS == -1 || m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS)
        {
            m_reconnectAttempts++;
            qDebug() << "\nПопытка переподключения" << m_reconnectAttempts << "/" << MAX_RECONNECT_ATTEMPTS;
            m_webSocket.abort();
            m_webSocket.open(m_serverUrl);
        } else {
            qCritical() << "\nПревышено максимальное число попыток подключения";
            m_reconnectTimer.stop();
            //QCoreApplication::quit();
        }
    }
}

void Client::onDisconnected()
{
    if (m_debug)
        qDebug() << "Соединение разорвано!";

    m_reconnectTimer.start();
}




void Client::onTextMessageReceived(const QString &message)
{
    if (m_debug)
        qDebug() << "Получено сообщение:" << message;

    emit messageReceived(message);
}

void Client::onErrorOccurred(QAbstractSocket::SocketError error)
{
    qWarning() << "Ошибка сокета:" << error << "-" << m_webSocket.errorString();
}

void Client::sendMessage(const QString &message)
{
    if (m_webSocket.state() == QAbstractSocket::ConnectedState)
        m_webSocket.sendTextMessage(message);
    else
        qWarning() << "Не удалось отправить сообщение: соединение не установлено";
}



