#include "alertmodel.h"
#include <algorithm> // std::min
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

AlertModel::AlertModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

int AlertModel::rowCount(const QModelIndex&) const { return m_rows.size(); }
int AlertModel::columnCount(const QModelIndex&) const { return ColCount; }

QVariant AlertModel::headerData(int s, Qt::Orientation o, int role) const {
    if (o!=Qt::Horizontal || role!=Qt::DisplayRole) return {};
    static const char* H[]={"Time","Msg","Src","SPort","Dst","DPort","Action","Sev.","Preview"};
    return (s>=0 && s<ColCount) ? H[s] : QVariant{};
}

QVariant AlertModel::data(const QModelIndex& i, int role) const {
    if (!i.isValid() || role!=Qt::DisplayRole) return {};
    const auto& r = m_rows[i.row()];
    switch(i.column()){
    case Time:     return r.timeStr;
    case Policy:   return r.policy;
    case Src:      return r.srcIp;
    case SPort:    return (int)r.srcPort;
    case Dst:      return r.dstIp;
    case DPort:    return (int)r.dstPort;
    case Action:   return r.action;
    case Severity: return r.severity;
    case Preview:  return r.preview;
    }
    return {};
}

void AlertModel::appendFromJson(const QJsonObject& j) {
    AlertRow r;
    const qint64 sec  = j["ts_sec"].toVariant().toLongLong();
    const qint64 usec = j["ts_usec"].toVariant().toLongLong();
    r.tsUsec = sec*1000000 + usec;
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(sec).addMSecs(usec/1000);
    r.timeStr  = dt.toLocalTime().toString("HH:mm:ss.zzz");

    r.policy   = j["policy"].toString();
    r.action   = j["action"].toString("ALERT");

    auto determineSeverity = [](const QString& msg) -> QString {
        const QString m = msg.toLower();
        if (m.contains("ddos") || m.contains("flood") || m.contains("icmp"))
            return "HIGH";
        if (m.contains("unauthorized") || m.contains("invalid") || m.contains("attack") || m.contains("intrusion"))
            return "CRITICAL";
        if (m.contains("speed") || m.contains("threshold") || m.contains("limit"))
            return "MEDIUM";
        if (m.contains("info") || m.contains("connected") || m.contains("normal"))
            return "LOW";
        return "-";
    };

    // 기존 JSON에 severity가 없을 경우 자동 판정
    if (j.contains("severity")) {
        r.severity = j["severity"].toString();
    } else {
        r.severity = determineSeverity(r.policy);
    }

    const auto js = j["src"].toObject();
    const auto jd = j["dst"].toObject();
    r.srcIp   = js["ip"].toString();
    r.srcPort = (quint16)js["port"].toInt();
    r.dstIp   = jd["ip"].toString();
    r.dstPort = (quint16)jd["port"].toInt();

    const QString b64 = j["payload_b64"].toString();
    if (!b64.isEmpty()) {
        const QByteArray bytes = QByteArray::fromBase64(b64.toUtf8());
        r.preview = hexPreview(bytes, 64);
    } else {
        r.preview = "";
    }
    r.raw = j;

    const int row = 0;
    beginInsertRows({}, row, row);
    m_rows.insert(m_rows.begin(),std::move(r));
    endInsertRows();

    // 과도 누적 방지 (선택)
    const int MAX_ROWS = 5000;
    if (m_rows.size() > MAX_ROWS) {
        const int removeCount = m_rows.size() - MAX_ROWS;
        beginRemoveRows({}, 0, removeCount - 1);
        m_rows.erase(m_rows.begin(), m_rows.begin() + removeCount);
        endRemoveRows();
    }
}

QJsonArray AlertModel::toJsonArray() const {
    QJsonArray arr;
    for (const auto& r : m_rows) {
        if (!r.raw.isEmpty()) {
            arr.append(r.raw);        // Telemetry에서 온 원본 JSON이 있으면 그걸 저장
        } else {
            QJsonObject j;
            j["type"]    = "ALERT";
            j["policy"]  = r.policy;
            j["action"]  = r.action;
            j["ts_usec"] = static_cast<qint64>(r.tsUsec);
            j["time"]    = r.timeStr;
            QJsonObject js, jd;
            js["ip"]     = r.srcIp;
            js["port"]   = static_cast<int>(r.srcPort);
            jd["ip"]     = r.dstIp;
            jd["port"]   = static_cast<int>(r.dstPort);
            j["src"]     = js;
            j["dst"]     = jd;
            j["severity"]= r.severity;
            j["preview"] = r.preview;
            arr.append(j);
        }
    }
    return arr;
}

QString AlertModel::hexPreview(const QByteArray& b, int maxLen) {
    const int n = std::min(maxLen, static_cast<int>(b.size()));
    QString s; s.reserve(n*3);
    for (int i=0;i<n;++i) s += QString("%1 ").arg((quint8)b[i],2,16,QChar('0'));
    return s.trimmed();
}
