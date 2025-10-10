#include "sessionform.h"
#include "ui_sessionform.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QJsonArray>

SessionForm::SessionForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SessionForm)
{
    ui->setupUi(this);
    m_model = new SessionModel(this);
    ui->tableSessions->setModel(m_model);
    ui->tableSessions->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableSessions->setSortingEnabled(true);

    initCharts();
    connect(ui->tableSessions->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &SessionForm::onTableRowChanged);
    connect(m_model, &QAbstractItemModel::modelReset, this, [this] {
        if (ui->tableSessions->selectionModel()) {
            connect(ui->tableSessions->selectionModel(),
                    &QItemSelectionModel::currentRowChanged,
                    this, &SessionForm::onTableRowChanged);
        }
    });
}

SessionForm::~SessionForm()
{
    delete ui;
}

void SessionForm::onFlow(const QJsonObject& evt) {
    // SessionModel의 업데이트 로직 그대로 활용
    m_model->upsertFromJson(evt);  // FLOW_UPDATE/END 처리
}

void SessionForm::onSummary(const QJsonObject& evt) {
    m_model->replaceFromSummary(evt); // SUMMARY 처리
    appendSamplesFromSummary(evt);
    if (!m_selectedKey.isEmpty())
        redrawCharts(m_selectedKey);
}

void SessionForm::initCharts() {
    // RTT 차트
    m_rttAckSeries = new QLineSeries(this);  m_rttAckSeries->setName("DATA RTT");
    auto *rttChart = new QChart();
    rttChart->addSeries(m_rttAckSeries);
    m_rttX = new QValueAxis();    m_rttX->setTitleText("Time (s)");
    m_rttY = new QValueAxis();    m_rttY->setTitleText("ms");
    rttChart->addAxis(m_rttX, Qt::AlignBottom);
    rttChart->addAxis(m_rttY, Qt::AlignLeft);
    m_rttAckSeries->attachAxis(m_rttX);
    m_rttAckSeries->attachAxis(m_rttY);

    auto *rttView = new QChartView(rttChart, ui->rttChartContainer);
    auto *rttLay  = new QVBoxLayout(ui->rttChartContainer); rttLay->setContentsMargins(0,0,0,0); rttLay->addWidget(rttView);

    // Throughput 차트
    m_tpA2BSeries = new QLineSeries(this);  m_tpA2BSeries->setName("A→B bps");
    m_tpB2ASeries = new QLineSeries(this);  m_tpB2ASeries->setName("B→A bps");
    auto *tpChart = new QChart();
    tpChart->addSeries(m_tpA2BSeries);
    tpChart->addSeries(m_tpB2ASeries);
    m_tpX = new QValueAxis();    m_tpX->setTitleText("Time (s)");
    m_tpY = new QValueAxis();    m_tpY->setTitleText("bps");
    tpChart->addAxis(m_tpX, Qt::AlignBottom);
    tpChart->addAxis(m_tpY, Qt::AlignLeft);
    for (auto* s : {m_tpA2BSeries, m_tpB2ASeries}) { s->attachAxis(m_tpX); s->attachAxis(m_tpY); }
    auto *tpView = new QChartView(tpChart, ui->tpChartContainer);
    auto *tpLay  = new QVBoxLayout(ui->tpChartContainer); tpLay->setContentsMargins(0,0,0,0); tpLay->addWidget(tpView);
}

// sample값을 만든다.
void SessionForm::appendSamplesFromSummary(const QJsonObject& summary) {
    const qint64 ts_ms = summary["ts_ns"].toVariant().toLongLong() / 1000000;
    const auto arr = summary["sessions"].toArray();
    for (const auto& v : arr) {
        const auto s = v.toObject();
        // flow key와 동일한 문자열 키를 만든다 (SessionModel::makeKey와 동일 형식)
        const auto fk = s["flow_key"].toObject();
        const auto key = SessionModel::makeKey(fk);
        Sample smp;
        smp.ts_ms       = ts_ms;
        // RTT
        const auto rtt  = s["rtt_ms"].toObject();
        smp.rtt_syn_ms  = rtt["syn"].toDouble();
        smp.rtt_ack_ms  = rtt["ack"].toDouble();
        // Throughput (bps)
        const auto tp   = s["throughput_bps"].toObject();
        smp.tp_a2b_bps  = tp["inst_a2b"].toDouble();   // 또는 ewma 사용
        smp.tp_b2a_bps  = tp["inst_b2a"].toDouble();

        auto& vec = m_hist[key];
        vec.push_back(smp);
        if (vec.size() > 2000) vec.remove(0, vec.size()-2000); // 메모리 제한 (옵션)
    }
}

// 테이블 선택 변경: 선택된 행의 KeyRole을 읽어들여 현재 키를 바꾸고 다시 그림
void SessionForm::onTableRowChanged(const QModelIndex& cur, const QModelIndex&) {
    if (!cur.isValid()) return;
    const auto key = cur.data(SessionModel::KeyRole).toString();
    m_selectedKey = key;
    redrawCharts(key);
}

void SessionForm::redrawCharts(const QString& key) {
    m_rttAckSeries->clear();
    m_tpA2BSeries->clear();
    m_tpB2ASeries->clear();

    const auto vec = m_hist.value(key);
    if (vec.isEmpty()) {
        // 비어있으면 축만 리셋(옵션)
        m_rttX->setRange(0.0, 60.0);
        m_rttY->setRange(0.0, 1.0);
        m_tpX->setRange(0.0, 60.0);
        m_tpY->setRange(0.0, 1.0);
        return;
    }

    const qint64 t0_ms = vec.first().ts_ms;   // 기준 시간(ms)
    double tEnd = 0.0;
    double maxRtt = 0.0;
    double maxTp  = 0.0;
    double lastSynRtt = -1.0;

    for (const auto& smp : vec) {
        const double t = (smp.ts_ms - t0_ms) / 1000.0; // 초 단위로 변환 (상대 시간)
        m_rttAckSeries->append(t, smp.rtt_ack_ms);
        m_tpA2BSeries->append(t, smp.tp_a2b_bps);
        m_tpB2ASeries->append(t, smp.tp_b2a_bps);

        maxRtt = std::max(maxRtt, smp.rtt_ack_ms);
        maxTp  = std::max({maxTp, smp.tp_a2b_bps, smp.tp_b2a_bps});
        if (smp.rtt_syn_ms > 0) lastSynRtt = smp.rtt_syn_ms;

        tEnd = t;
    }

    if (ui->lblSynRtt) {
        if (lastSynRtt >= 0.0)
            ui->lblSynRtt->setText(QString("SYN RTT: %1 ms").arg(lastSynRtt, 0, 'f', 2));
        else
            ui->lblSynRtt->setText("SYN RTT: N/A");
    }

    // 축 범위 갱신 (여유 20%)
    const double xmin = 0.0;
    const double xmax = std::max(5.0, tEnd);
    m_rttX->setRange(xmin, xmax);
    m_tpX->setRange(xmin, xmax);

    m_rttY->setRange(0.0, std::max(1.0, maxRtt * 1.2));
    m_tpY->setRange(0.0, std::max(1.0, maxTp  * 1.2));
}

