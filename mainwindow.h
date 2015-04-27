#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#define llib_ignore_byte_typedef

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void autosave_handler();

    void on_pushButton_clicked();

    void on_actionLoad_triggered();

    void on_actionSave_triggered();

    void on_actionAutosave_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
