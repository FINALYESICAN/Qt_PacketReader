#include "Tcpreceiver.h"
#include <QtEndian>

TcpReceiver::TcpReceiver(QObject* p): QObject(p) {
}

bool TcpReceiver::start(const QString& host, quint16 port) {
    if (m_sock) {                 // 기존 소켓 정리
        m_sock->disconnect(this);
        m_sock->close();
        m_sock->deleteLater();
        m_sock = nullptr;
    }

    m_sock = new QTcpSocket(this);
    connect(m_sock,&QTcpSocket::readyRead, this, &TcpReceiver::onReadyRead);
    connect(m_sock,&QTcpSocket::disconnected, this, &TcpReceiver::onDisconnected);
    m_buf.clear();
    m_sock->connectToHost(host, port);
    if(!m_sock->waitForConnected(3000)){
        emit status("connect failed: " + m_sock->errorString());
        return false;
    }
    emit status(QString("Connected to %1:%2").arg(host).arg(port));
    return true;
}
void TcpReceiver::stop() {
    if (m_sock) {
        m_sock->disconnect(this);
        m_sock->close();
        m_sock->deleteLater();
        m_sock = nullptr;
    }
    m_buf.clear();
    emit status("Stopped");
}

void TcpReceiver::onReadyRead() {
    m_buf += m_sock->readAll();
    while (tryExtractOne()) {}
}

void TcpReceiver::onDisconnected() {
    emit status("Probe disconnected");
}

bool TcpReceiver::tryExtractOne() {
    if (m_buf.size() < 4) return false;
    const uchar* p = reinterpret_cast<const uchar*>(m_buf.constData());
    uint32_t len = qFromBigEndian(*reinterpret_cast<const uint32_t*>(p));
    if (m_buf.size() < (int)(4+len)) return false;
    QByteArray jsonBytes = m_buf.mid(4, len);
    m_buf.remove(0, 4+len);

    QJsonParseError e{};
    auto doc = QJsonDocument::fromJson(jsonBytes, &e);
    if (e.error != QJsonParseError::NoError || !doc.isObject()) {
        emit status("JSON parse error: " + e.errorString());
        return (m_buf.size() >= 4);
    }
    emit eventArrived(doc.object());
    return (m_buf.size() >= 4);
}

bool TcpReceiver::sendJson(const QJsonObject& obj){
    if (!m_sock) return false;
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray buf;
    buf.resize(4);
    // 4바이트 길이를 big-endian으로 버퍼에 기록
    qToBigEndian<quint32>(static_cast<quint32>(body.size()),
                          reinterpret_cast<uchar*>(buf.data()));
    buf += body;

    const qint64 n = m_sock->write(buf);
    // 필요하면 즉시 전송 보장:
    // m_sock->flush(); m_sock->waitForBytesWritten(100);
    return (n == buf.size());
}
