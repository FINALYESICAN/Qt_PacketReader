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

    // PacketForm 생성자 끝부분에 추가
    auto upd = [this]{ refreshFilterLabelsAndCount(); };

    connect(m_model, &QAbstractItemModel::rowsInserted, this, [upd]{ upd(); });
    connect(m_model, &QAbstractItemModel::rowsRemoved,  this, [upd]{ upd(); });
    connect(m_model, &QAbstractItemModel::modelReset,   this, [upd]{ upd(); });

    // 프록시 측 변화(필터 적용, 정렬 등)도 카운트가 바뀔 수 있음
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, [upd]{ upd(); });
    connect(m_proxy, &QAbstractItemModel::rowsRemoved,  this, [upd]{ upd(); });
    connect(m_proxy, &QAbstractItemModel::modelReset,   this, [upd]{ upd(); });
    connect(m_proxy, &QAbstractItemModel::layoutChanged,this, [upd]{ upd(); });
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
    refreshFilterLabelsAndCount();
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

    refreshFilterLabelsAndCount();
}


void PacketForm::on_ResetButton_clicked()
{
    //reset filter and label
    ui->leSrcIp->clear(); ui->leSrcPort->clear();
    ui->leDstIp->clear(); ui->leDstPort->clear();
    on_FilterButton_clicked();
}

void PacketForm::refreshFilterLabelsAndCount()
{
    // 1) 카운트: 전체 vs 필터 후
    const int total    = m_model  ? m_model->rowCount(QModelIndex())  : 0;
    const int filtered = m_proxy  ? m_proxy->rowCount(QModelIndex())  : 0;

    // 2) 현재 필터 문자열 구성 (UI의 lineEdit 값으로 재구성)
    auto ipPort = [](const QString& ip, const QString& port){
        if (ip.isEmpty() && port.isEmpty()) return QString("-");
        if (ip.isEmpty())  return QString(":%1").arg(port);
        if (port.isEmpty())return ip;
        return QString("%1:%2").arg(ip, port);
    };

    const QString srcStr = ipPort(ui->leSrcIp->text().trimmed(),
                                  ui->leSrcPort->text().trimmed());
    const QString dstStr = ipPort(ui->leDstIp->text().trimmed(),
                                  ui->leDstPort->text().trimmed());

    // (선택) OR/AND 모드 표기 — 체크박스가 있다면 이름에 맞춰 사용
    // const bool orMode = ui->cbOr->isChecked();
    // const QString modeStr = orMode ? "OR" : "AND";

    // 3) 라벨에 반영
    //   - packetFilterLabel: "Src=..., Dst=..." 형식
    //   - packetCountLabel : "count(filtered/total)" 형식
    const QString filterText =
        QString("Src=%1, Dst=%2").arg(srcStr, dstStr);
    // + QString(", Mode=%1").arg(modeStr); // (선택) OR/AND 표기

    ui->Filter->setText(filterText);
    ui->PacketCountLabel->setText(QString("count: %1 / %2").arg(filtered).arg(total));
}

