#ifndef MRTWINDOW_H
#define MRTWINDOW_H

#include <QtGui/QMainWindow>
#include <imbl/mrtShutter.h>
#include <imbl/shutter1A.h>

namespace Ui {
    class MRTwindow;
}

class MRTwindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MRTwindow(QWidget *parent = 0);
    ~MRTwindow();

private:
  MrtShutter * shut;
  Shutter1A * shut1A;
  Ui::MRTwindow *ui;

  bool stopme;

private slots:

  void updateShutterStatus();
  void setExposure(int exp);
  void trigShutter();
  void onStartStop();

};

#endif // MRTWINDOW_H
