#include "sessionform.h"
#include "ui_sessionform.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QScrollBar>

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
    const qint64 window_ms = 20000;

    //이번 summary session key
    QSet<QString> present;

    const auto arr = summary["sessions"].toArray();
    for (const auto& v : arr) {
        const auto s = v.toObject();
        // flow key와 동일한 문자열 키를 만든다 (SessionModel::makeKey와 동일 형식)
        const auto fk = s["flow_key"].toObject();
        const auto key = SessionModel::makeKey(fk);
        present.insert(key);

        const auto bytes_dir  = s["bytes_dir"].toObject();
        const quint64 curA = bytes_dir["a2b"].toVariant().toULongLong();
        const quint64 curB = bytes_dir["b2a"].toVariant().toULongLong();

        LastTotals &last = m_lastBytes[key];

        Sample smp;
        smp.ts_ms       = ts_ms;
        // RTT
        const auto rtt  = s["rtt_ms"].toObject();
        if (curA == last.a2b && curB == last.b2a) {
            smp.rtt_syn_ms = 0.0;
            smp.rtt_ack_ms = 0.0;
        } else {
            smp.rtt_syn_ms = rtt["syn"].toDouble();
            smp.rtt_ack_ms = rtt["ack"].toDouble();
        }

        // Throughput (bps) 이전과 비교해서 들어왔을때만 늘리기
        const auto tp   = s["throughput_bps"].toObject();
        smp.tp_a2b_bps  = (curA == last.a2b)?0.0:tp["inst_a2b"].toDouble();   // 또는 ewma 사용
        smp.tp_b2a_bps  = (curB == last.a2b)?0.0:tp["inst_b2a"].toDouble();

        auto& vec = m_hist[key];

        // cutoff
        const qint64 cutoff = ts_ms-window_ms;
        int keepStart = 0;
        while (keepStart < vec.size() && vec[keepStart].ts_ms < cutoff) ++keepStart;
        if (keepStart > 0) vec.remove(0, keepStart);
        if (vec.isEmpty() || vec.last().ts_ms != ts_ms){
            vec.push_back(smp);
        }

        if (vec.size() > 2000) vec.remove(0, vec.size()-2000); // 메모리 제한 (옵션)

        // 직전 누적값 갱신
        last.a2b = curA;
        last.b2a = curB;
    }
    for (auto it = m_hist.begin(); it != m_hist.end(); ++it) {
        const QString& key = it.key();
        if (present.contains(key)) continue;

        auto& vec = it.value();
        const qint64 cutoff = ts_ms - window_ms;
        int keepStart = 0;
        while (keepStart < vec.size() && vec[keepStart].ts_ms < cutoff) ++keepStart;
        if (keepStart > 0) vec.remove(0, keepStart);

        if (!vec.isEmpty() && vec.last().ts_ms == ts_ms) continue; // 중복 방지

        Sample zero{};
        zero.ts_ms = ts_ms;
        zero.rtt_syn_ms = 0.0;
        zero.rtt_ack_ms = 0.0;
        zero.tp_a2b_bps = 0.0;
        zero.tp_b2a_bps = 0.0;

        vec.push_back(zero);
        if (vec.size() > 2000) vec.remove(0, vec.size() - 2000);
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
    const double window_s = 20;

    if (vec.isEmpty()) {
        // 비어있으면 축만 리셋(옵션)
        m_rttX->setRange(0.0, window_s);
        m_rttY->setRange(0.0, 1.0);
        m_tpX->setRange(0.0, window_s);
        m_tpY->setRange(0.0, 1.0);
        if (ui->lblSynRtt) ui->lblSynRtt->setText("SYN RTT: N/A");
        return;
    }

    const qint64 last_ms = vec.last().ts_ms;
    const qint64 base_ms = last_ms - static_cast<qint64>(window_s * 1000.0);

    double maxRtt = 0.0;
    double maxTp  = 0.0;
    double lastSynRtt = -1.0;

    for (const auto& smp : vec) {
        double t = (smp.ts_ms - base_ms) / 1000.0; // 초 단위로 변환 (상대 시간)
        if(t<0.0) t=0.0;
        if (t > window_s) t = window_s;

        m_rttAckSeries->append(t, smp.rtt_ack_ms);
        m_tpA2BSeries->append(t, smp.tp_a2b_bps);
        m_tpB2ASeries->append(t, smp.tp_b2a_bps);

        maxRtt = std::max(maxRtt, smp.rtt_ack_ms);
        maxTp  = std::max({maxTp, smp.tp_a2b_bps, smp.tp_b2a_bps});
        if (smp.rtt_syn_ms > 0) lastSynRtt = smp.rtt_syn_ms;
    }

    // X축: 항상 0~20초
    m_rttX->setRange(0.0, window_s);
    m_tpX->setRange(0.0, window_s);

    // Y축 여유 20%
    m_rttY->setRange(0.0, std::max(1.0, maxRtt * 1.2));
    m_tpY->setRange(0.0, std::max(1.0, maxTp  * 1.2));

    // SYN RTT 라벨 업데이트
    if (ui->lblSynRtt) {
        if (lastSynRtt >= 0.0)
            ui->lblSynRtt->setText(QString("SYN RTT: %1 ms").arg(lastSynRtt, 0, 'f', 2));
        else
            ui->lblSynRtt->setText("SYN RTT: N/A");
    }
}

