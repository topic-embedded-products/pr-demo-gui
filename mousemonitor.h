#ifndef MOUSEMONITOR_H
#define MOUSEMONITOR_H

#include <QObject>

class QTimer;

class MouseMonitor : public QObject
{
    Q_OBJECT
public:
    explicit MouseMonitor(QObject *parent = 0);
    
protected:
    QTimer* actionTimer;
    bool mouse_down;
    bool is_idle;
    bool eventFilter(QObject *obj, QEvent *event);
    void start_timer();
    void stop_timer();
    void show_cursor();
    void hide_cursor();

signals:

    
public slots:
    void timeout();
    
};

#endif // MOUSEMONITOR_H
