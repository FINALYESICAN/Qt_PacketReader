#ifndef ALERTMODEL_H
#define ALERTMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>

struct AlertRow {
    qint64   tsUsec = 0;      // epoch usec
    QString  timeStr;         // "HH:mm:ss.zzz"
    QString  policy;          // rule msg
    QString  action;          // "ALERT"
    QString  srcIp;
    quint16  srcPort = 0;
    QString  dstIp;
    quint16  dstPort = 0;
    QString  severity;        // optional (없으면 "-" 세팅)
    QString  preview;         // payload head hex preview
    QJsonObject raw;          // 필요하면 원문 보관
};

class AlertModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Col { Time, Policy, Src, SPort, Dst, DPort, Action, Severity, Preview, ColCount };

    explicit AlertModel(QObject* parent=nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // ALERT JSON 1건 추가
    void appendFromJson(const QJsonObject& j);

private:
    QVector<AlertRow> m_rows;

    static QString hexPreview(const QByteArray& b, int maxLen);
};

#endif // ALERTMODEL_H
