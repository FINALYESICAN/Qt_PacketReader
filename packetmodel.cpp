#include "packetmodel.h"

#include <QDateTime>
#include <QJsonArray>
#include <QByteArray>

PacketModel::PacketModel(QObject* parent) : QAbstractTableModel(parent) {}

QVariant PacketModel::headerData(int section, Qt::Orientation o, int role) const {
    if (o!=Qt::Horizontal || role!=Qt::DisplayRole) return {};
    static const char* H[] = {
        "Time","Dir","Proto","Src","SPort","Dst","DPort","Len","Flags","Preview"
    };
    return (section>=0 && section<ColCount) ? H[section] : QVariant{};
}

QVariant PacketModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid()) return {};
    const auto& r = m_rows[idx.row()];

    if(role == Qt::DisplayRole){
        switch (idx.column()) {
        case Time:    return formatTimeUS(r.tsUsec);
        case Dir:     return r.dir;
        case Proto:   return r.proto;
        case Src:     return r.src;
        case Sport:   return (int)r.sport;
        case Dst:     return r.dst;
        case Dport:   return (int)r.dport;
        case Len:     return r.caplen;
        case Flags:   return r.flags;
        case Preview: return r.preview;
        }
    }else if (role == RawTsRole) {
        return (qlonglong)r.tsUsec;             // 더블클릭 요청에 쓰고 싶으면 사용
    } else if (role == IdRole) {
        return (qulonglong)r.id;                // ★ PACKET_REQ용
    } else if (role == ProtoNumRole) {
        return r.protoNum;
    }
    return {};
}

void PacketModel::addFromJson(const QJsonObject& pkt) {
    if (pkt["type"].toString() != "PACKET") return;

    PacketRow row;

    // id
    row.id     = pkt["id"].toVariant().toULongLong();
    // 타임스탬프 (µs)
    row.tsUsec = pkt["ts_usec"].toVariant().toLongLong();

    // flow_key
    const QJsonObject fk = pkt["flow_key"].toObject();
    const int proto = fk["proto"].toInt();
    row.protoNum = proto;
    row.proto = protoToString(proto);
    row.src   = fk["src_ip"].toString();
    row.dst   = fk["dst_ip"].toString();
    row.sport = (quint16)fk["src_port"].toInt();
    row.dport = (quint16)fk["dst_port"].toInt();

    // 방향/길이/플래그
    row.dir    = pkt["dir"].toString();           // 없으면 빈 문자열
    row.caplen = pkt["caplen"].toInt();
    row.flags  = pkt["l4"].toObject()["flags"].toString();

    // payload preview (선택 필드)
    const QString b64 = pkt["payload_b64"].toString();
    qDebug()<<b64;
    if (!b64.isEmpty()) {
        auto bytes = fromBase64ToBytes(b64);
        row.preview = hexPreview(bytes, 64);
    }

    // append + trim
    const int r = m_rows.size();
    beginInsertRows({}, r, r);
    m_rows.push_back(std::move(row));
    endInsertRows();
    trimIfNeeded();
}

void PacketModel::trimIfNeeded() {
    if (m_maxRows<=0 || m_rows.size()<=m_maxRows) return;
    const int over = m_rows.size() - m_maxRows;
    beginRemoveRows({}, 0, over-1);
    for (int i=0;i<over;i++) m_rows.removeFirst();
    endRemoveRows();
}

QString PacketModel::formatTimeUS(qint64 tsUsec) {
    qint64 sec = tsUsec/1000000;
    qint64 us  = tsUsec%1000000;
    return QDateTime::fromSecsSinceEpoch(sec).toLocalTime()
               .toString("HH:mm:ss") + QString(".%1").arg(us/1000,3,10,QChar('0'));
}

QString PacketModel::protoToString(int p) {
    if (p==6)  return "TCP";
    if (p==17) return "UDP";
    if (p==1)  return "ICMP";
    return QString::number(p);
}

QByteArray PacketModel::fromBase64ToBytes(const QString& b64) {
    return QByteArray::fromBase64(b64.toUtf8());
}

// "xx xx ... | ASCII" 형태의 간단 프리뷰
QString PacketModel::hexPreview(const QByteArray& bytes, int maxBytes) {
    const int n = qMin(maxBytes, bytes.size());
    QString hex; hex.reserve(n*3 + 3 + n);
    for (int i=0;i<n;i++) hex += QString("%1 ").arg((unsigned char)bytes[i],2,16,QChar('0')).toUpper();
    hex += "| ";
    for (int i=0;i<n;i++) {
        unsigned char c = (unsigned char)bytes[i];
        hex += (c>=32 && c<127) ? QChar(c) : '.';
    }
    if (bytes.size()>n) hex += " …";
    return hex;
}
