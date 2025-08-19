#ifndef TOAST_H
#define TOAST_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QScreen>
#include <QGuiApplication>
#include <QWindow>
#include <QCursor>

class Toast : public QWidget
{
    Q_OBJECT
public:
    explicit Toast(const QString& message, QWidget* parent = nullptr);

    // 在右下角显示，持续 durationMs 毫秒；anchor 为参考窗口（可为空，空则使用当前屏幕）
    void showWithDuration(int durationMs = 2000, QWidget* anchor = nullptr);

    // 便捷静态函数：一行调用
    static void showBottomRight(const QString& message, int durationMs = 2000, QWidget* anchor = nullptr);

protected:
    void showEvent(QShowEvent*) override;

private:
    void positionAtBottomRight(QWidget* anchor);
    void startFadeIn();
    void startFadeOut();

private:
    QLabel*             label_        {nullptr};
    QPropertyAnimation* fade_         {nullptr};
    QTimer*             closeTimer_   {nullptr};
    QWidget*            anchor_       {nullptr};
};

#endif // TOAST_H
