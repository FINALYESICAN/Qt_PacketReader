#ifndef PACKETMODEL_H
#define PACKETMODEL_H

#include <QWidget>
#include <QAbstractTableModel>
#include <QVector>
#include <QJsonObject>

struct PacketRow {
    qint64  tsUsec = 0;      // µs
    QString proto; int protoNum=0;           // "TCP"/"UDP"/number
    QString src;  quint16 sport = 0;
    QString dst;  quint16 dport = 0;
    int     caplen = 0;      // captured length
    QString flags;           // TCP flags e.g. "S","PA","F"...
    QString preview;         // payload hex/ASCII preview (truncated)
    quint64 id = 0;
};

class PacketModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Col { Time, Proto, Src, Sport, Dst, Dport, Len, Flags, Preview, ColCount };
    enum Roles { RawTsRole = Qt::UserRole+1, IdRole, ProtoNumRole };
    explicit PacketModel(QObject* parent=nullptr);
    int rowCount(const QModelIndex&) const override { return m_rows.size(); }
    int columnCount(const QModelIndex&) const override { return ColCount; }
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QVariant data(const QModelIndex& idx, int role) const override;

    // type == "PACKET" JSON을 한 행으로 추가
    void addFromJson(const QJsonObject& pkt);

    // 메모리 보호: 최근 N개만 유지(앞쪽 삭제)
    void setMaxRows(int n) { m_maxRows = n; trimIfNeeded(); }

private:
    QVector<PacketRow> m_rows;
    int m_maxRows = 10000;

    void trimIfNeeded();
    static QString formatTimeUS(qint64 tsUsec);
    static QString protoToString(int p);
    static QByteArray fromBase64ToBytes(const QString& b64);
    static QString hexPreview(const QByteArray& bytes, int maxBytes=64);
};

#endif // PACKETMODEL_H
