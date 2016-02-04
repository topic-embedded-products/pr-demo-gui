#include "mousemonitor.h"
#include <QEvent>
#include <QTimer>
#include <QApplication>

#include <QDebug>

MouseMonitor::MouseMonitor(QObject *parent) :
    QObject(parent),
    mouse_down(false),
    is_idle(false)
{
    actionTimer = new QTimer(this);
    actionTimer->setSingleShot(true);
    connect(actionTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    start_timer();
}


bool MouseMonitor::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::MouseMove:
        if (!mouse_down)
            start_timer();
        break;
    case QEvent::MouseButtonPress:
        mouse_down = true;
        stop_timer();
        show_cursor();
        break;
    case QEvent::MouseButtonRelease:
        mouse_down = false;
        start_timer();
        break;
    case QEvent::FocusIn:
        /* When a windows re-appears, it doesn't generate any mouse events,
         * so trigger the timer to hide it */
        if (!mouse_down)
            start_timer();
        break;
    default:
        break; /* Avoids silly -Wswitch warnings */
    }
    return QObject::eventFilter(obj, event);
}

void MouseMonitor::start_timer()
{
    show_cursor();
    actionTimer->start(1000);
}

void MouseMonitor::stop_timer()
{
    actionTimer->stop();
}

void MouseMonitor::show_cursor()
{
    if (is_idle)
    {
        is_idle = false;
        QApplication::restoreOverrideCursor();
    }
}

void MouseMonitor::hide_cursor()
{
    if (!is_idle)
    {
        is_idle = true;
        QApplication::setOverrideCursor(Qt::BlankCursor);
    }
}

void MouseMonitor::timeout()
{
    hide_cursor();
}

