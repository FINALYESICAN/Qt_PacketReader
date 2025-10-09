#ifndef ALERTMODEL_H
#define ALERTMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QJsonObject>

struct AlertRow {
    QString severity, ruleId, category, message;
    QString flow;
    qint64  tsUsec=0;
};

class AlertModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Col { Time, Severity, RuleID, Category, Message, Flow, ColCount };
    explicit AlertModel(QObject* parent=nullptr);

    int rowCount(const QModelIndex&) const override { return m_rows.size(); }
    int columnCount(const QModelIndex&) const override { return ColCount; }
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QVariant data(const QModelIndex& idx, int role) const override;

    void addFromJson(const QJsonObject& evt); // type==ALERT

private:
    QVector<AlertRow> m_rows;
};

#endif // ALERTMODEL_H
