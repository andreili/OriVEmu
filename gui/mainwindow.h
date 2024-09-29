#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QImage>
#include <QLabel>
#include <QCloseEvent>
#include "sim_top.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    SIM_TOP *sim;
    QImage *image_scr;
    int cur_zoom;
    int scr_h, scr_w;
    QLabel *status_screen;

    static void thread_start_draw();
    static void thread_resize();
public slots:
    void resize_window();
    void draw();
    void keyPressEvent(QKeyEvent *evt);
    void keyReleaseEvent(QKeyEvent *evt);
    void closeEvent(QCloseEvent *evt);
signals:
    void start_draw();
    void start_resize();
private slots:
    void on_actionx1_triggered();
    void on_actionx2_triggered();
    void on_actionx3_triggered();
    void on_actionx4_triggered();
};
#endif // MAINWINDOW_H
