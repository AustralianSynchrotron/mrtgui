#ifndef MRTWINDOW_H
#define MRTWINDOW_H

#include <QtGui/QMainWindow>
#include <imbl/mrtShutterGui.h>
#include <imbl/shutter1A.h>
#include <QSettings>

namespace Ui {
    class MRTwindow;
}

class GeneralFastShutter;

class MRTwindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MRTwindow(QWidget *parent = 0);
    ~MRTwindow();

private:

  GeneralFastShutter * shutfast;
  Shutter1A * shut1A;
  Ui::MRTwindow *ui;

  static QSettings localSettings;

  QProcess * procBefore;
  QProcess * procAfter;

  bool stopme;

private slots:

  void updateShutterStatus();
  void updateStartStatus();

  void onStartStop();

  void saveConfig();
  void loadConfig();

  void onBrowseBefore();
  void onBrowseAfter();
  void onChangedBefore();
  void onChangedAfter();
  void onExecBefore();
  void onExecAfter();

};

#endif // MRTWINDOW_H
