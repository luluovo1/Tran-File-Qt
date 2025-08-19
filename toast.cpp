#include "toast.h"

Toast::Toast(const QString& message, QWidget* parent)
    : QWidget{parent}
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background-color: rgba(50, 50, 50, 200); color: white; border-radius: 10px; padding: 8px 12px;");

    label_ = new QLabel(this);
    label_->setWordWrap(true);                   // 自动换行
    //label_->setTextFormat(Qt::PlainText);        // 按纯文本处理，保留 \n 等
    label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label_->setText(message);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->addWidget(label_);

    // 基本动画
    fade_ = new QPropertyAnimation(this, "windowOpacity", this);
    fade_->setDuration(180);

    closeTimer_ = new QTimer(this);
    closeTimer_->setSingleShot(true);
    connect(closeTimer_, &QTimer::timeout, this, &Toast::startFadeOut);

    resize(280, sizeHint().height());
}

void Toast::positionAtBottomRight(QWidget* anchor)
{
    anchor_ = anchor;
    const QScreen* screen = nullptr;
    if (anchor_ && anchor_->windowHandle()) {
        screen = anchor_->windowHandle()->screen();
    }
    if (!screen) {
        // 使用鼠标所在屏，否则主屏
        screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) screen = QGuiApplication::primaryScreen();
    }
    const QRect avail = screen->availableGeometry();
    const int margin = 16;
    move(avail.right() - width() - margin,
         avail.bottom() - height() - margin);
}

void Toast::startFadeIn()
{
    setWindowOpacity(0.0);
    fade_->stop();
    fade_->setStartValue(0.0);
    fade_->setEndValue(1.0);
    fade_->start();
}

void Toast::startFadeOut()
{
    fade_->stop();
    fade_->setStartValue(windowOpacity());
    fade_->setEndValue(0.0);
    connect(fade_, &QPropertyAnimation::finished, this, &QWidget::close);
    fade_->start();
}

void Toast::showEvent(QShowEvent*) {
    // 如果外部未设置锚点，则默认按当前屏幕
    positionAtBottomRight(anchor_);
}

void Toast::showWithDuration(int durationMs, QWidget* anchor) {
    // 统一行尾并自适应高度，支持 \n 与制表符展开
    QString text = label_->text();
    text.replace("\r\n", "\n");
    label_->setText(text);

    const int maxW = 320;             // 提示最大宽度
    setFixedWidth(maxW);
    const int textW = maxW - 24;      // 减去左右内边距

    QFontMetrics fm(label_->font());
    const QRect br = fm.boundingRect(QRect(0, 0, textW, 100000),
                                     Qt::TextWordWrap | Qt::TextExpandTabs,
                                     text);
    const int h = br.height() + 16;   // 加上下内边距
    setFixedHeight(h);

    positionAtBottomRight(anchor);
    startFadeIn();
    show();
    closeTimer_->start(durationMs);
}

void Toast::showBottomRight(const QString& message, int durationMs, QWidget* anchor)
{
    auto* t = new Toast(message);
    t->showWithDuration(durationMs, anchor);
}
