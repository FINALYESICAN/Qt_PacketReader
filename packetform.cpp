#include "packetform.h"
#include "ui_packetform.h"
#include "packetmodel.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QCheckBox>

PacketForm::PacketForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PacketForm)
{
    ui->setupUi(this);
    m_model = new PacketModel(this);
    m_proxy = new packetfilterproxy(this);
    m_proxy->setSourceModel(m_model);
    ui->tablePacket->setModel(m_proxy);
    //ui->tablePacket->setModel(m_model);
    ui->tablePacket->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tablePacket->setSortingEnabled(true);
}

PacketForm::~PacketForm()
{
    delete ui;
}

// 스크롤 위치 보존해서 1건 추가
void PacketForm::appendWithScrollGuard(const QJsonObject& evt)
{
    auto *view = ui->tablePacket;
    auto *sb   = view->verticalScrollBar();

    // 1) 현재 스크롤 상태 저장
    const int oldMax = sb->maximum();
    const int oldVal = sb->value();
    const bool stickBottom = (oldVal >= oldMax - 2); // 거의 바닥에 붙어 있던 경우

    // 2) 깜빡임/점프 최소화
    view->setUpdatesEnabled(false);
    m_model->addFromJson(evt);          // 내부에서 trimIfNeeded()도 호출
    view->setUpdatesEnabled(true);

    // 3) 복원 / 자동 스크롤
    if (stickBottom) {
        view->scrollToBottom();
    } else {
        // 상단 트리밍으로 최대값이 바뀌었을 수 있으니 차이만큼 보정
        const int newMax = sb->maximum();
        const int delta  = newMax - oldMax;         // 음수/양수 모두 가능
        int target = oldVal + delta;
        target = std::max(0, std::min(target, newMax));
        sb->setValue(target);
    }
}

void PacketForm::onPacket(const QJsonObject& evt) {
    appendWithScrollGuard(evt);
}

//더블 클릭시 패킷 페이로드 데이터 가져오기.
void PacketForm::on_tablePacket_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QModelIndex srcIdx = m_proxy->mapToSource(index);
    const quint64 id = m_model->data(
                                  m_model->index(srcIdx.row(), PacketModel::Time),
                                  PacketModel::IdRole
                                  ).toULongLong();
    if(!id) return;

    QJsonObject req{
        {"type","PACKET_REQ"},
        {"id", static_cast<qint64>(id)},
        {"want_bytes", 512}
    };
    //signal slot
    emit packetReq(req);
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

void PacketForm::on_FilterButton_clicked()
{
    //setting filter ip/port
    if (!m_proxy) return;
    m_proxy->setSrcIp(ui->leSrcIp->text());
    m_proxy->setSrcPort(ui->leSrcPort->text());
    m_proxy->setDstIp(ui->leDstIp->text());
    m_proxy->setDstPort(ui->leDstPort->text());
}


void PacketForm::on_ResetButton_clicked()
{
    //reset filter and label
    ui->leSrcIp->clear(); ui->leSrcPort->clear();
    ui->leDstIp->clear(); ui->leDstPort->clear();
    on_FilterButton_clicked();
}

