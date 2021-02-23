#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <functional>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dbc_auto_logger("DB Client")
{
    ui->setupUi(this);

    //Attach the logger
    dbc_auto_logger.attach(std::bind(&MainWindow::logs_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    //Initialize core
    if(!dbc.initialize()){
        //TODO show window with the reason
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::logs_callback(DBC::LogType type, DBC::LogLevel level, const std::string & str){
    //Create color with level
    QColor color(0,0,0);
    switch(level){
    case DBC::LogLevel::DEBUG: color.setBlue(200); break;
    case DBC::LogLevel::INFO: color.setGreen(100); break;
    case DBC::LogLevel::WARNING: color = QColor(239, 127, 26); break;
    case DBC::LogLevel::ERROR: color.setRed(250); break;
    default: break;
    }

    //Construct string from str & type
    QString qstr;
    if(type != DBC::LogType::NOTYPE){
        qstr.append(QString(DBC::Logger::type2str[type]));
        qstr.append(QString(": "));
    }
    qstr.append(QString(str.c_str()));

    //Queue it to be run by the UI
    QMetaObject::invokeMethod(this, "append_log", Qt::QueuedConnection,
                              Q_ARG(QColor, color), Q_ARG(QString, qstr));
}

void MainWindow::append_log(QColor color, QString qstr){
    ui->textBrowser->setTextColor(color);
    ui->textBrowser->append(qstr);
}

void MainWindow::update_p2p(){
    ui->status_p2p_connections->setText(QString(dbc.mP2P.mClientSocket));
    ui->textBrowser->append(qstr);
}

void MainWindow::on_actionClose_triggered() {
    QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}
