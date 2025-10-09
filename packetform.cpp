#include "packetform.h"
#include "ui_packetform.h"
#include "packetmodel.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>

PacketForm::PacketForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PacketForm)
{
    ui->setupUi(this);
    m_model = new PacketModel(this);
    ui->tablePacket->setModel(m_model);
    ui->tablePacket->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tablePacket->setSortingEnabled(true);
}

PacketForm::~PacketForm()
{
    delete ui;
}

void PacketForm::onPacket(const QJsonObject& evt) {
    m_model->addFromJson(evt);
}

//더블 클릭시 패킷 페이로드 데이터 가져오기.
void PacketForm::on_tablePacket_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    const int r = index.row();
    const quint64 id = m_model->data(m_model->index(index.row(), PacketModel::Time), PacketModel::IdRole).toULongLong();
    if(!id) {
        qDebug()<<"no id for row"<<r;
        return;
    }

    QJsonObject req{
        {"type","PACKET_REQ"},
        {"id", static_cast<qint64>(id)},
        {"want_bytes", 512}
    };
    //signal slot
    packetReq(req);
}

// PACKET_DATA를 받아서 다이얼로그로 보여주기
void PacketForm::onPacketData(const QJsonObject& data) {
    if (data["type"].toString()!="PACKET_DATA") return;
    const QByteArray raw = QByteArray::fromBase64(data["payload_b64"].toString().toUtf8());

    // 간단한 상세 다이얼로그
    QDialog dlg(this);
    dlg.setWindowTitle("Packet detail");
    QVBoxLayout lay(&dlg);
    QLabel l1(QString("caplen=%1, payload_len=%2")
                  .arg(data["caplen"].toInt())
                  .arg(data["payload_len"].toInt()));
    QPlainTextEdit hex;
    hex.setReadOnly(true);

    // 헥스/ASCII 변환
    QString out; out.reserve(raw.size()*3 + raw.size());
    for (int i=0;i<raw.size();++i) {
        if (i && (i%16)==0) out += '\n';
        out += QString("%1 ").arg((unsigned char)raw[i],2,16,QChar('0')).toUpper();
    }
    out += "\n\n";
    for (int i=0;i<raw.size();++i) {
        if (i && (i%64)==0) out += '\n';
        unsigned char c = (unsigned char)raw[i];
        out += (c>=32 && c<127) ? QChar(c) : '.';
    }
    hex.setPlainText(out);

    lay.addWidget(&l1);
    lay.addWidget(&hex);
    dlg.resize(720, 480);
    dlg.exec();
}

