#pragma once

/**
 * @file toast.h
 * @brief 应用内 Toast 消息提示组件
 *
 * 参考 old Vue 项目的 Layout.vue toast 设计：
 * - 右上角固定位置显示
 * - 三种类型：success/error/info
 * - 自动消失（默认3秒）
 * - 淡入淡出动画
 */

#include <QWidget>
#include <QString>

class QLabel;

class Toast : public QWidget
{
    Q_OBJECT

public:
    enum Type { Success, Error, Info };

    explicit Toast(QWidget *parent, const QString &message, Type type = Info, int durationMs = 3000);

    /**
     * @brief 静态便捷方法：创建并显示 Toast
     * @param parent    父窗口（用于定位）
     * @param message   提示消息
     * @param type      消息类型
     * @param durationMs 显示时长（毫秒）
     */
    static void show(QWidget *parent, const QString &message, Type type = Info, int durationMs = 3000);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void initUi(const QString &message, Type type);

    int m_durationMs;
};
