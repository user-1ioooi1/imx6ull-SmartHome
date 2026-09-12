#ifndef TOASTMESSAGE_H
#define TOASTMESSAGE_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class ToastMessage : public QWidget
{
    Q_OBJECT
public:
    enum Level { Info, Warning, Error };

    explicit ToastMessage(QWidget* parent = nullptr);


    static void showToast(QWidget* parent,
                          const QString& text,
                          Level level = Error,
                          int durationMs = 1500);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QLabel* m_label;
    QGraphicsOpacityEffect* m_opacity;
    QPropertyAnimation* m_anim;
};

#endif // TOASTMESSAGE_H
