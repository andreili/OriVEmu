#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPainter>

MainWindow* p_mw;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    p_mw = this;
    ui->setupUi(this);
    QStringList args = QCoreApplication::arguments();
    const char* argv[10];
    for (size_t i=0 ; i<1 ; ++i)
    {
        if (i < args.size())
        {
            argv[i] = args[i].toStdString().c_str();
        }
        else
        {
            argv[i] = NULL;
        }
    }
    sim = new SIM_TOP(args.size(), argv, thread_start_draw, thread_resize);
    connect(this, SIGNAL(start_draw()), SLOT(draw()));
    connect(this, SIGNAL(start_resize()), SLOT(resize_window()));

    image_scr = new QImage();
    cur_zoom = 1;
    ui->actionx1->setChecked(true);

    status_screen = new QLabel();
    //ui->statusbar->addPermanentWidget(status_screen);

    // TODO
    sim->load_rom1("./ROMs/ROM1-321.BIN");
    sim->load_rom2("./ROMs/ROM2-321.ROM");
    sim->load_rom_disk("./ROMs/romdisk1.bin");

    sim->run_cont();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::thread_start_draw()
{
    emit p_mw->start_draw();
}

void MainWindow::thread_resize()
{
    p_mw->scr_h = p_mw->sim->get_height();
    p_mw->scr_w = p_mw->sim->get_width();
    if (p_mw->image_scr->width() != p_mw->scr_w)
    {
        delete p_mw->image_scr;
        p_mw->image_scr = new QImage(p_mw->scr_w, p_mw->scr_h, QImage::Format_RGB32);
        emit p_mw->start_resize();
        //p_mw->sim->set_screen((uint32_t*)p_mw->image_scr->data_ptr());
    }
}

void MainWindow::resize_window()
{
    setMaximumSize(scr_w * cur_zoom, scr_h * cur_zoom);
    ui->label->setMaximumSize(scr_w * cur_zoom, scr_h * cur_zoom);
}

void MainWindow::draw()
{
    memcpy(image_scr->bits(), sim->get_screen(), image_scr->sizeInBytes());
    QImage img = image_scr->scaled(scr_w * cur_zoom, scr_h * cur_zoom);
    this->ui->label->setPixmap(QPixmap::fromImage(img));
    status_screen->setText(QString("%1:%2").arg(scr_w, scr_h));
}

void MainWindow::on_actionx1_triggered()
{
    ui->actionx2->setChecked(false);
    ui->actionx3->setChecked(false);
    ui->actionx4->setChecked(false);
    cur_zoom = 1;
    resize_window();
}


void MainWindow::on_actionx2_triggered()
{
    ui->actionx1->setChecked(false);
    ui->actionx3->setChecked(false);
    ui->actionx4->setChecked(false);
    cur_zoom = 2;
    resize_window();
}


void MainWindow::on_actionx3_triggered()
{
    ui->actionx1->setChecked(false);
    ui->actionx2->setChecked(false);
    ui->actionx4->setChecked(false);
    cur_zoom = 3;
    resize_window();
}


void MainWindow::on_actionx4_triggered()
{
    ui->actionx1->setChecked(false);
    ui->actionx2->setChecked(false);
    ui->actionx3->setChecked(false);
    cur_zoom = 4;
    resize_window();
}

void MainWindow::keyPressEvent(QKeyEvent *evt)
{
    if (this->isActiveWindow())
    {
        sim->key_press(evt->key());
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *evt)
{
    if (this->isActiveWindow())
    {
        sim->key_release(evt->key());
    }
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
    sim->stop();
    evt->accept();
}

