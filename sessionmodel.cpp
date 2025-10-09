#include "sessionmodel.h"
#include <QDateTime>
#include <QJsonArray>

SessionModel::SessionModel(QObject* parent): QAbstractTableModel(parent) {}

QVariant SessionModel::headerData(int section, Qt::Orientation o, int role) const {
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    static const char* H[]={"Proto","Src","SPort","Dst","DPort","A→B Bytes","B→A Bytes","Last(TS)","State"};
    return (section>=0 && section<ColCount) ? H[section] : QVariant{};
}

QVariant SessionModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || role!=Qt::DisplayRole) return {};
    const auto& r = m_rows[idx.row()];
    switch (idx.column()) {
    case Proto:    return r.proto;
    case Src:      return r.src;
    case Sport:    return (int)r.sport;
    case Dst:      return r.dst;
    case Dport:    return (int)r.dport;
    case BytesA2B: return (qulonglong)r.bytesA2B;
    case BytesB2A: return (qulonglong)r.bytesB2A;
    case LastTs: {
        qint64 sec = r.tsUsec/1000000;
        qint64 us  = r.tsUsec%1000000;
        return QDateTime::fromSecsSinceEpoch(sec).toLocalTime()
                   .toString("HH:mm:ss") + QString(".%1").arg(us/1000,3,10,QChar('0'));
    }
    case State:    return r.state;
    }
    return {};
}

QString SessionModel::makeKey(const QJsonObject& fk) {
    // proto|src|sport|dst|dport  (라즈베리에서 이미 정규화된 순서라면 그대로)
    return QString("%1|%2|%3|%4|%5")
        .arg(fk["proto"].toInt())
        .arg(fk["src_ip"].toString())
        .arg(fk["src_port"].toInt())
        .arg(fk["dst_ip"].toString())
        .arg(fk["dst_port"].toInt());
}

void SessionModel::upsertFromJson(const QJsonObject& evt) {
    if (!evt.contains("flow_key") || !evt.contains("body")) return;
    auto fk   = evt["flow_key"].toObject();
    auto body = evt["body"].toObject();
    QString key = makeKey(fk);

    SessionRow row;
    row.proto  = (fk["proto"].toInt()==6) ? "TCP" : QString::number(fk["proto"].toInt());
    row.src    = fk["src_ip"].toString();
    row.sport  = (quint16)fk["src_port"].toInt();
    row.dst    = fk["dst_ip"].toString();
    row.dport  = (quint16)fk["dst_port"].toInt();
    row.bytesA2B = body["bytes_a2b"].toVariant().toULongLong();
    row.bytesB2A = body["bytes_b2a"].toVariant().toULongLong();
    row.tsUsec   = evt["ts_usec"].toVariant().toLongLong();
    if (evt["type"].toString()=="FLOW_END") {
        QString reason = body["reason"].toString();
        row.state = "ENDED(" + (reason.isEmpty()?"?":reason) + ")";
    }

    auto it = m_index.find(key);
    if (it == m_index.end()) {
        int r = m_rows.size();
        beginInsertRows({}, r, r);
        m_rows.push_back(row);
        m_index.insert(key, r);
        endInsertRows();
    } else {
        int r = it.value();
        m_rows[r] = row;
        emit dataChanged(index(r,0), index(r,ColCount-1));
    }
}

void SessionModel::replaceFromSummary(const QJsonObject& evt) {
    if (evt["type"].toString() != "SUMMARY") return;
    const QJsonArray arr = evt["sessions"].toArray();

    beginResetModel();
    m_rows.clear();
    m_index.clear();

    for (const auto& v : arr) {
        const QJsonObject js = v.toObject();
        const QJsonObject key = js["key"].toObject();
        const QJsonObject pkts = js["pkts"].toObject();
        const QJsonObject bytes_dir = js["bytes_dir"].toObject(); // Telemetry에 추가 권장

        SessionRow row;
        const int proto = key["proto"].toInt();
        row.proto = (proto==6) ? "TCP" : QString::number(proto);

        // Telemetry JSON에 sip_str/dip_str을 이미 넣었음
        row.src   = key["sip_str"].toString();
        row.dst   = key["dip_str"].toString();
        row.sport = (quint16)key["sport"].toInt();
        row.dport = (quint16)key["dport"].toInt();

        // 방향별 바이트(없다면 0으로)
        row.bytesA2B = bytes_dir["a2b"].toVariant().toULongLong();
        row.bytesB2A = bytes_dir["b2a"].toVariant().toULongLong();

        // 타임스탬프: 일단 SUMMARY의 ts_ns를 사용(µs로 변환)
        row.tsUsec = evt["ts_ns"].toVariant().toLongLong() / 1000;

        row.state = js["state"].toString();

        m_index.insert(makeKey(QJsonObject{
                           {"proto", proto},
                           {"src_ip", row.src}, {"src_port", (int)row.sport},
                           {"dst_ip", row.dst}, {"dst_port", (int)row.dport}
                       }), m_rows.size());
        m_rows.push_back(row);
    }
    endResetModel();
}
