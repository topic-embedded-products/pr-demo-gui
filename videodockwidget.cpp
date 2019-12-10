#include "videodockwidget.h"
#include "ui_videodockwidget.h"

VideoDockWidget::VideoDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::VideoDockWidget)
{
    ui->setupUi(this);
}

VideoDockWidget::~VideoDockWidget()
{
    delete ui;
}
