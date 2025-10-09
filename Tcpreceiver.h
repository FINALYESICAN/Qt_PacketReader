#ifndef TCPRECEIVER_H
#define TCPRECEIVER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

class TcpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit TcpReceiver(QObject* parent=nullptr);
    bool start(const QString& host,quint16 port);
    void stop();
    bool sendJson(const QJsonObject& obj);

signals:
    void eventArrived(const QJsonObject& evt);   // FLOW_*/ALERT 공통
    void status(const QString& msg);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_sock = nullptr;
    QByteArray m_buf;
    bool tryExtractOne();
};

#endif //TCPRECEIVER_H
