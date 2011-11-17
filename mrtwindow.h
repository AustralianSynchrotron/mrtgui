#ifndef MRTWINDOW_H
#define MRTWINDOW_H

#include <QtGui/QMainWindow>
#include <imbl/mrtShutterGui.h>
#include <imbl/shutter1A.h>
#include <QSettings>

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

  static QSettings localSettings;

  bool stopme;

private slots:

  void updateShutterStatus();
  void onStartStop();

  void saveConfig();
  void loadConfig();

};

#endif // MRTWINDOW_H
