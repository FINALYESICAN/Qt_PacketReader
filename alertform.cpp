#include "alertform.h"
#include "ui_alertform.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>

alertForm::alertForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::alertForm)
{
    ui->setupUi(this);

    m_model = new AlertModel(this);
    ui->tableAlerts->setModel(m_model);
    ui->tableAlerts->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableAlerts->horizontalHeader()
        ->setSectionResizeMode(AlertModel::Preview, QHeaderView::Interactive);
    ui->tableAlerts->setColumnWidth(AlertModel::Preview, 380);
    ui->tableAlerts->setSortingEnabled(true);
    ui->tableAlerts->sortByColumn(AlertModel::Time, Qt::DescendingOrder);
}

alertForm::~alertForm()
{
    delete ui;
}

void alertForm::appendWithScrollGuard(const QJsonObject& evt)
{
    auto *view = ui->tableAlerts;
    auto *sb   = view->verticalScrollBar();

    // 1) 현재 스크롤 상태 저장
    const int oldMax = sb->maximum();
    const int oldVal = sb->value();
    const bool stickBottom = (oldVal >= oldMax - 2); // 거의 바닥에 붙어 있던 경우

    // 2) 깜빡임/점프 최소화
    view->setUpdatesEnabled(false);
    m_model->appendFromJson(evt);          // 내부에서 trimIfNeeded()도 호출
    view->setUpdatesEnabled(true);

    // 3) 복원 / 자동 스크롤
    if (stickBottom) {
        view->scrollToBottom();
    } else {
        // 상단 트리밍으로 최대값이 바뀌었을 수 있으니 차이만큼 보정
        const int newMax = sb->maximum();
        const int delta  = newMax - oldMax;         // 음수/양수 모두 가능
        int target = oldVal + delta;
        target = std::max(0, std::min(target, newMax));
        sb->setValue(target);
    }
}

void alertForm::onAlert(const QJsonObject& evt) {
    // type 필터링 (혹시 다른 타입이 들어올 수도 있으니까)
    if (evt["type"].toString() != "ALERT") return;
    appendWithScrollGuard(evt);
}

// 로그 저장
QString alertForm::suggestedSavePath() const {
    // 라인에디트에 사용자가 미리 적어둔 경로가 있으면 우선
    const QString fromEdit = ui->LogLineEdit->text().trimmed();
    if (!fromEdit.isEmpty()) return fromEdit;

    // 없으면 홈/문서 폴더 기준 기본 파일명 제안
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString base = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QDir d(docs.isEmpty() ? QDir::homePath() : docs);
    return d.filePath(QString("snort_alerts_%1.json").arg(base));
}

void alertForm::on_LogButton_clicked()
{
    const QString suggested = suggestedSavePath();

    // 2) 파일 다이얼로그 열기 (사용자가 원하는 위치/이름 지정 가능)
    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Save Alerts as JSON"),
        suggested,
        tr("JSON files (*.json);;All Files (*)")
        );
    if (path.isEmpty()) return; // 취소

    // 3) 모델에서 JSON 뽑기
    const QJsonArray arr = m_model->toJsonArray();   // <-- 새로 추가한 함수 사용
    const QJsonDocument doc(arr);

    // 4) 디렉터리 보장
    QDir().mkpath(QFileInfo(path).absolutePath());

    // 5) 파일로 저장
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Save Failed"),
                              tr("Cannot open file:\n%1").arg(path));
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();

    // 6) 라인에디트에 최종 경로 업데이트 + 안내
    ui->LogLineEdit->setText(path);
    QMessageBox::information(this, tr("Saved"),
                             tr("Saved %1 alerts to:\n%2")
                                 .arg(QString::number(arr.size()), path));
}
