#include "alertform.h"
#include "ui_alertform.h"
#include <QHeaderView>
#include <QScrollBar>

alertForm::alertForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::alertForm)
{
    ui->setupUi(this);

    m_model = new AlertModel(this);
    ui->tableAlerts->setModel(m_model);
    ui->tableAlerts->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableAlerts->setSortingEnabled(true);
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
