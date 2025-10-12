#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 연결
    m_TR = new TcpReceiver(this);
    connect(m_TR, &TcpReceiver::eventArrived, this, &MainWindow::onEvent);
    connect(m_TR, &TcpReceiver::status,       this, &MainWindow::onStatus);
    m_TR->start(ip,55555);//192.168.2.234, 55555;

    // 세션 데이터 도착
    connect(this, &MainWindow::flowEvent, ui->session,&SessionForm::onFlow);
    connect(this, &MainWindow::summaryEvent, ui->session, &SessionForm::onSummary);
    // 경고 데이터 도착.
    connect(this, &MainWindow::alertEvent, ui->alert, &alertForm::onAlert);
    // 패킷 데이터 도착.
    connect(this, &MainWindow::packetEvent,ui->packet,&PacketForm::onPacket);
    // 패킷 데이터 요청/가져오기.
    connect(this, &MainWindow::packetDataEvent,ui->packet,&PacketForm::onPacketData);
    connect(ui->packet, &PacketForm::packetReq,this,[this](const QJsonObject& req){
        m_TR->sendJson(req);
        qDebug()<<"send Packet Req id = "<<req["id"];
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onStatus(const QString& s) {
    ui->statusbar->showMessage(s, 5000);
}
//received sth
void MainWindow::onEvent(const QJsonObject& evt) {
    const QString type = evt["type"].toString();
    if (type=="FLOW_UPDATE" || type=="FLOW_END") {
        emit flowEvent(evt);
    } else if (type=="ALERT") {
        emit alertEvent(evt);
    } else if (type=="SUMMARY"){
        emit summaryEvent(evt);
    } else if (type=="PACKET"){
        emit packetEvent(evt);
    } else if (type=="PACKET_DATA"){
        emit packetDataEvent(evt);
    }
}
