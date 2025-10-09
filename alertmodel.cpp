#include "alertmodel.h"
#include <QDateTime>

AlertModel::AlertModel(QObject* parent): QAbstractTableModel(parent) {}

QVariant AlertModel::headerData(int section, Qt::Orientation o, int role) const {
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    static const char* H[]={"Time","Severity","RuleID","Category","Message","Flow"};
    return (section>=0 && section<ColCount) ? H[section] : QVariant{};
}

QVariant AlertModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || role!=Qt::DisplayRole) return {};
    const auto& r = m_rows[idx.row()];
    switch (idx.column()) {
    case Time: {
        qint64 sec = r.tsUsec/1000000, us = r.tsUsec%1000000;
        return QDateTime::fromSecsSinceEpoch(sec).toLocalTime()
                   .toString("HH:mm:ss") + QString(".%1").arg(us/1000,3,10,QChar('0'));
    }
    case Severity: return r.severity;
    case RuleID:   return r.ruleId;
    case Category: return r.category;
    case Message:  return r.message;
    case Flow:     return r.flow;
    }
    return {};
}

void AlertModel::addFromJson(const QJsonObject& evt) {
    if (evt["type"].toString()!="ALERT") return;
    auto fk   = evt["flow_key"].toObject();
    auto body = evt["body"].toObject();
    AlertRow r;
    r.tsUsec   = evt["ts_usec"].toVariant().toLongLong();
    r.severity = body["severity"].toString();
    r.ruleId   = body["rule_id"].toString();
    r.category = body["category"].toString();
    r.message  = body["message"].toString();
    r.flow = QString("%1:%2 → %3:%4")
                 .arg(fk["src_ip"].toString())
                 .arg(fk["src_port"].toInt())
                 .arg(fk["dst_ip"].toString())
                 .arg(fk["dst_port"].toInt());
    int row = m_rows.size();
    beginInsertRows({}, row, row);
    m_rows.push_back(std::move(r));
    endInsertRows();
}
