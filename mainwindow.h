#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include "Tcpreceiver.h"
#include "sessionform.h"
#include "packetform.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void summaryEvent(const QJsonObject&);      // SUMMARY
    void alertEvent(const QJsonObject&);        // ALERT
    void packetEvent(const QJsonObject&);       // PACKET (추가 예정시)
    void packetDataEvent(const QJsonObject&);   // PACKET DATA;
    void reportEvent(const QJsonObject&);       // Session Report

private slots:
    void onEvent(const QJsonObject& evt);
    void onStatus(const QString& s);

private:
    Ui::MainWindow *ui;
    TcpReceiver *m_TR;
    QString ip = QStringLiteral("192.168.3.51"); //raspbot
    //QString ip = QStringLiteral("192.168.2.44"); //raspberry
    SessionForm*    m_sessionform = nullptr;
};
#endif // MAINWINDOW_H
