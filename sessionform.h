#ifndef SESSIONFORM_H
#define SESSIONFORM_H

#include <QWidget>
#include <QHash>
#include <QVector>
#include <QJsonObject>
#include "sessionmodel.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

//for chart
struct Sample {
    qint64 ts_ms;
    double rtt_syn_ms;
    double rtt_ack_ms;
    double tp_a2b_bps;
    double tp_b2a_bps;
};

namespace Ui {
class SessionForm;
}

class SessionForm : public QWidget
{
    Q_OBJECT

public:
    explicit SessionForm(QWidget *parent = nullptr);
    ~SessionForm();

public slots:
    void onFlow(const QJsonObject& evt);
    void onSummary(const QJsonObject& evt);
    void onTableRowChanged(const QModelIndex& cur, const QModelIndex& prev);

private:
    Ui::SessionForm *ui;
    SessionModel* m_model = nullptr;
    // session key별 히스토리 버퍼
    QHash<QString, QVector<Sample>> m_hist;
    QString m_selectedKey;

    // 차트 구성요소
    QLineSeries *m_rttSynSeries=nullptr, *m_rttAckSeries=nullptr;
    QValueAxis*     m_rttX = nullptr;
    QValueAxis*     m_rttY = nullptr;
    QLineSeries *m_tpA2BSeries=nullptr, *m_tpB2ASeries=nullptr;
    QValueAxis*     m_tpX = nullptr;
    QValueAxis*     m_tpY = nullptr;

    void initCharts();
    static QString makeKey(const QJsonObject& keyObj); // Telemetry SUMMARY의 key에서 동일 규칙
    void appendSamplesFromSummary(const QJsonObject& summary);
    void redrawCharts(const QString& key);
};

#endif // SESSIONFORM_H
