#ifndef SESSIONMODEL_H
#define SESSIONMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>
#include <QJsonObject>

struct SessionRow {
    QString proto;
    QString src;  quint16 sport=0;
    QString dst;  quint16 dport=0;
    quint64 bytesA2B=0, bytesB2A=0;
    qint64  tsUsec=0;
    QString state; // "", "ENDED(RST/IDLE/FIN)"
};

class SessionModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Col { Proto, Src, Sport, Dst, Dport, BytesA2B, BytesB2A, LastTs, State, ColCount };
    explicit SessionModel(QObject* parent=nullptr);

    int rowCount(const QModelIndex&) const override { return m_rows.size(); }
    int columnCount(const QModelIndex&) const override { return ColCount; }
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QVariant data(const QModelIndex& idx, int role) const override;

    // flow_key를 문자열 키로 만들어서 map
    void upsertFromJson(const QJsonObject& evt); // FLOW_UPDATE/END 처리
    void replaceFromSummary(const QJsonObject& summary);

private:
    static QString makeKey(const QJsonObject& flowKey);
    QVector<SessionRow> m_rows;
    QHash<QString,int> m_index; // key -> row
};

#endif // SESSIONMODEL_H
