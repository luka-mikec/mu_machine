#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QMessageBox>
#include <regex>
#include "mu_machine.h"

using namespace std;
using boost::tokenizer;
using boost::char_separator;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    try
    {
        std::string text = this->ui->plainTextEdit->toPlainText().toStdString();
        std::string text2 = this->ui->plainTextEdit_2->toPlainText().toStdString();

        text = regex_replace(text, regex("#[^#]*#"), " ");
        text2 = regex_replace(text2, regex("#[^#]*#"), " ");

        char_separator<char> sep(" \t\r,", "{}|()+=\n");
        tokenizer<char_separator<char>> tokens(text, sep);
        tokenizer<char_separator<char>> tokens2(text2, sep);
        if (all_of(full(tokens), [](string tok) { return tok == "\n"; }))
              //  || all_of(full(tokens2), [](auto tok) { return *tok == "\n"; }))
            throw runtime_error("code must be non-empty.");

        ui->plainTextEdit_3->setPlainText(QString::fromStdString("run (...) = " + scast<string>(
            compile_and_run(tokens, tokens2)
        )));
    }
    catch (runtime_error e)
    {
        QMessageBox Msgbox;

        Msgbox.setText(e.what());
        Msgbox.exec();
    }

    std::cout <<  std::endl;
}
