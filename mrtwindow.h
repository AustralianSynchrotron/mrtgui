#ifndef MRTWINDOW_H
#define MRTWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui/QHBoxLayout>
#include <imbl/shutter1A.h>
#include <imbl/mrtShutter.h>
#include <imbl/mrtShutterGui.h>
#include <fastShutterUi.h>
#include <fastShutterBase.h>
#include <QSettings>


class GeneralFastShutter : public QWidget {
  Q_OBJECT;

public :

  enum ExposureMode {
    TIME=0,
    SOFT=1
  };

  bool isConnected() const;
  ExposureMode exposureMode() const;
  int repeats() const;
  int progress() const;
  bool canStart() const;
  bool valuesOK() const;

public slots:

  void start(bool beAwareOf1A=false);
  void trig(bool wait=false);
  void stop();
  void setExposureMode(ExposureMode val);
  void setRepeats(int val);

signals:

  void connectionChanged(bool connection);
  void exposureModeChanged(GeneralFastShutter::ExposureMode);
  void repeatsChanged(int);
  void progressChanged(int);
  void canStartChanged(bool);
  void valuesOKchanged(bool);

private slots:

  void updateExposureMode();

private:

  QHBoxLayout *horizontalLayout;
  MrtShutterGui * mrtshut;
  FastShutterWidget * fastshut;

public :

  enum KnownShutters {
    MRT=0,
    IS01=1
  };

  static const QString shutterName(KnownShutters shut);
  GeneralFastShutter(QWidget * parent = 0 );
  void setShutter(KnownShutters shut);


};




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
  void onShutterChange();

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
