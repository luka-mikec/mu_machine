#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QMessageBox>
#include <regex>
#include "mu_machine.h"
#include <fstream>


using namespace std;
using boost::tokenizer;
using boost::char_separator;


#define full_gui_functionality

#ifdef full_gui_functionality
    #include <QFileDialog>
    #include <QTimer>
    bool autosave = false;
    string old_adress = "sample.mu";
#endif


void MainWindow::autosave_handler()
{
#ifdef full_gui_functionality

    if (autosave && old_adress != "")
    {
        ofstream f(old_adress);
        f << ui->plainTextEdit->toPlainText().toStdString();

    }
    this->setWindowTitle(QString::fromStdString(
        "mu_machine: " +
        (old_adress == "" ? string("None") : old_adress)
    ));
#endif
}



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#ifdef full_gui_functionality
    old_adress = (QDir::currentPath() + QString("sample.mu")).toStdString();
    ifstream sample_code(old_adress);
    if (!sample_code.is_open())
    {
        old_adress = "/host/dev/mu_machine/sample.mu";
        sample_code.open(old_adress);
    }

    if (sample_code.is_open())
    {
        ui->actionAutosave->setChecked(true);
        autosave = true;

        stringstream ff;
        ff << sample_code.rdbuf();
        auto qtxt = QString::fromStdString(ff.str());
        ui->plainTextEdit->setPlainText(qtxt);
    }
    else
    {
        autosave = false;
        old_adress = "";

        ui->plainTextEdit->setPlainText("");
    }

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(autosave_handler()));
    autosave_handler();
    timer->start(3000);

#endif

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


void MainWindow::on_actionLoad_triggered()
{
#ifdef full_gui_functionality
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    if(dialog.exec()) {
        ifstream f(dialog.selectedFiles()[0].toStdString());
        stringstream ff;
        ff << f.rdbuf();
        auto qtxt = QString::fromStdString(ff.str());
        ui->plainTextEdit->setPlainText(qtxt);
        old_adress = dialog.selectedFiles()[0].toStdString();
    }
#endif
}

void MainWindow::on_actionSave_triggered()
{
#ifdef full_gui_functionality
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
    if(dialog.exec()) {
        ofstream f(dialog.selectedFiles()[0].toStdString());
        f << ui->plainTextEdit->toPlainText().toStdString();
        old_adress = dialog.selectedFiles()[0].toStdString();
    }
#endif
}

void MainWindow::on_actionAutosave_triggered()
{
#ifdef full_gui_functionality
    autosave = ui->actionAutosave->isChecked();
#endif
}
