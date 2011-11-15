#ifndef MRTWINDOW_H
#define MRTWINDOW_H

#include <QtGui/QMainWindow>
#include <imbl/mrtShutterGui.h>
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
  MrtShutterGui * shut;
  Shutter1A * shut1A;
  Ui::MRTwindow *ui;

  bool stopme;

private slots:

  void updateShutterStatus();
  void onStartStop();

};

#endif // MRTWINDOW_H
