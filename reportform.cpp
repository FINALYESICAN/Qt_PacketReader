#include "reportform.h"
#include "ui_reportform.h"

ReportForm::ReportForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ReportForm)
{
    ui->setupUi(this);
}

ReportForm::~ReportForm()
{
    delete ui;
}

QString ReportForm::humanBytes(qulonglong n) {
    static const char* U[] = {"B","KB","MB","GB","TB"};
    int i = 0; double v = double(n);
    while (v >= 1024.0 && i < 4) { v /= 1024.0; ++i; }
    return QString::number(v, 'f', (i==0?0:1)) + " " + U[i];
}

void ReportForm::onReport(const QJsonObject& rep)
{
    // TelemetryServer::push_session_report() 포맷에 맞춰 파싱
    // { type, reason, flow_key{...}, summary{...}, time, ... }
    const auto fk = rep.value("flow_key").toObject();
    const auto sm = rep.value("summary").toObject();

    const auto src = QString("%1:%2").arg(fk.value("src_ip").toString())
                         .arg(fk.value("src_port").toInt());
    const auto dst = QString("%1:%2").arg(fk.value("dst_ip").toString())
                         .arg(fk.value("dst_port").toInt());

    const auto totalPkts  = sm.value("total_pkts").toVariant().toULongLong();
    const auto totalBytes = sm.value("total_bytes").toVariant().toULongLong();
    const auto avgBps     = sm.value("avg_bps").toDouble();
    const bool rttIsNull  = sm.value("avg_rtt_ms").isNull();
    const double avgRtt   = rttIsNull ? -1.0 : sm.value("avg_rtt_ms").toDouble();
    const auto retrans    = sm.value("retrans_pkts").toVariant().toULongLong();

    QString text;
    text += "===== Session Summary =====\n";
    if (rep.contains("reason") && !rep.value("reason").toString().isEmpty())
        text += "Reason: " + rep.value("reason").toString() + "\n";
    text += QString("Session: %1 <-> %2\n").arg(src, dst);
    text += "Total Packets: " + QString::number(totalPkts) + "\n";
    text += "Data Transferred: " + humanBytes(totalBytes) + "\n";
    text += QString("Avg RTT: %1\n").arg((avgRtt >= 0.0)
                                             ? QString("%1 ms").arg(QString::number(avgRtt,'f',2)) : "N/A");
    text += QString("Avg Throughput: %1 B/s\n").arg(QString::number(avgBps,'f',1));
    text += "Retransmissions: " + QString::number(retrans) + "\n";
    text += "===========================\n";

    ui->plainTextEdit->appendPlainText(text);
}
