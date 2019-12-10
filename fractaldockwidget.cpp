#include "fractaldockwidget.h"
#include "ui_fractaldockwidget.h"

FractalDockWidget::FractalDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FractalDockWidget)
{
    ui->setupUi(this);
}

FractalDockWidget::~FractalDockWidget()
{
    delete ui;
}
