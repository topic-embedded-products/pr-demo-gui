#ifndef FRACTALDOCKWIDGET_H
#define FRACTALDOCKWIDGET_H

#include <QDockWidget>

namespace Ui {
class FractalDockWidget;
}

class FractalDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FractalDockWidget(QWidget *parent = 0);
    ~FractalDockWidget();

    Ui::FractalDockWidget *ui;
};

#endif // FRACTALDOCKWIDGET_H
