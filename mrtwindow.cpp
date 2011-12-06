#include "mrtwindow.h"
#include <imbl/mrtShutter.h>
#include "ui_mrtwindow.h"


QSettings MRTwindow::localSettings(QDir::homePath() + "/.mrtgui", QSettings::IniFormat);

MRTwindow::MRTwindow(QWidget *parent) :
    QMainWindow(parent),
    shut(new MrtShutterGui),
    shut1A(new Shutter1A),
    ui(new Ui::MRTwindow)
{
  ui->setupUi(this);

  ui->axis2->setVisible(ui->use2nd->isChecked());
  ui->placeShutter->addWidget(shut);

  connect(shut->component(), SIGNAL(progressChanged(int)), SLOT(updateShutterStatus()));
  connect(shut->component(), SIGNAL(canStartChanged(bool)), SLOT(updateShutterStatus()));
  connect(shut->component(), SIGNAL(valuesOKchanged(bool)), SLOT(updateShutterStatus()));
  connect(shut->component(), SIGNAL(connectionChanged(bool)), SLOT(updateShutterStatus()));

  connect(ui->start, SIGNAL(clicked()), SLOT(onStartStop()));

  loadConfig();
  connect(ui->axis1, SIGNAL(settingChanged()), SLOT(saveConfig()));
  connect(ui->axis2, SIGNAL(settingChanged()), SLOT(saveConfig()));
  connect(ui->use2nd, SIGNAL(toggled(bool)), SLOT(saveConfig()));
  connect(ui->after, SIGNAL(currentIndexChanged(int)), SLOT(saveConfig()));

  ui->progressBar->setHidden(true);

  //shut->component()->setRepeats(1);

}

MRTwindow::~MRTwindow() {
  delete ui;
  delete shut;
  delete shut1A;
}


void MRTwindow::updateShutterStatus() {

  if ( ! shut->component()->isConnected() ) {

    ui->start->setEnabled(false);
    ui->disabledShutter->setVisible(false);
    ui->badShutter->setVisible(false);

  } else {

    bool canStart = shut->component()->canStart();
    bool valuesOK = shut->component()->valuesOK();
    bool exposing = ui->progressBar->isVisible();

    ui->disabledShutter->setVisible( ! canStart );
    ui->badShutter->setVisible( ! valuesOK );
    ui->start->setEnabled( exposing || (canStart && valuesOK) );

  }

}


void MRTwindow::onStartStop() {

  if ( shut->component()->progress() ) { // in prog
    stopme = true;
    shut->component()->stop();
    return;
  }

  // get initial positions
  const int points1 = ui->axis1->points();
  const int points2 = ui->use2nd->isChecked() ? ui->axis2->points() : 1;
  const double initPos1 = ui->axis1->motor->motor()->getUserPosition();
  const double initPos2 = ui->use2nd->isChecked() ?
                          ui->axis2->motor->motor()->getUserPosition()  :  0 ;


  if ( shut->component()->exposureMode() != MrtShutter::SOFT ) {
    shut->component()->setExposureMode(MrtShutter::SOFT);
    qtWait(shut->component(), SIGNAL(exposureModeChanged(MrtShutter::ExposureMode)), 500);
  }
  if ( shut->component()->repeats() != points1 * points2 ) {
    shut->component()->setRepeats(points1 * points2);
    qtWait(shut->component(), SIGNAL(repeatsChanged(int)), 500);
  }
  shut->component()->start(true);
  qtWait(shut->component(), SIGNAL(progressChanged(int)), 500);
  if ( shut->component()->exposureMode() != MrtShutter::SOFT ||
       shut->component()->repeats() != points1 * points2 ||
       ! shut->component()->progress() ) {
    return;
  }

  double start1 = ui->axis1->start();
  double end1 = ui->axis1->end();
  if (ui->axis1->mode() == Axis::REL) {
    start1 += initPos1;
    end1 += initPos1;
  }
  double start2 = ui->axis2->start();
  double end2 = ui->axis2->end();
  if (ui->axis2->mode() == Axis::REL) {
    start2 += initPos2;
    end2 += initPos2;
  }

  stopme = false;
  shut->setEnabled(false);
  ui->start->setText("Stop");
  ui->progressBar->setValue(0);
  ui->progressBar->setMaximum(points1*points2);
  ui->progressBar->setVisible(true);

  int curpoint = 0;
  for( int pnt2 = 0 ; pnt2 < points2 ; pnt2++ ) {

    if(ui->use2nd->isChecked())
      ui->axis2->motor->motor()->goUserPosition(
          start2 + ( pnt2 * ( end2 - start2 ) ) / (points2 - 1), QCaMotor::STARTED );

    if (stopme)
      break;

    for( int pnt1 = 0 ; pnt1 < points1 ; pnt1++ ) {

      ui->axis1->motor->motor()->goUserPosition(
          start1 + ( pnt1 * ( end1 - start1 ) ) / (points1 - 1), QCaMotor::STARTED);

      if (stopme)
        break;

      ui->axis1->motor->motor()->wait_stop();
      if ( ui->use2nd->isChecked() )
        ui->axis2->motor->motor()->wait_stop();

      //int curProg = shut->component()->progress();
      shut->component()->trig(true);
      //if ( shut->component()->progress() != curProg + 1 )

      ui->progressBar->setValue(++curpoint);

      if (stopme)
        break;

    }

    if (stopme)
      break;

  }

  shut->component()->stop();
  shut1A->close(false);

  // after scan positioning
  if ( ui->after->currentText() == "Start position" ) {
    ui->axis1->motor->motor()->goUserPosition(start1, QCaMotor::STARTED);
    if ( ui->use2nd->isChecked() )
      ui->axis2->motor->motor()->goUserPosition(start2);
  } else if ( ui->after->currentText() == "Prior position" ) {
    ui->axis1->motor->motor()->goUserPosition(initPos1, QCaMotor::STARTED);
    if ( ui->use2nd->isChecked() )
      ui->axis2->motor->motor()->goUserPosition(initPos2);
  }
  ui->axis1->motor->motor()->wait_stop();
  if ( ui->use2nd->isChecked() )
    ui->axis2->motor->motor()->wait_stop();

  shut->setEnabled(true);
  ui->start->setText("Start");
  ui->progressBar->setVisible(false);


}


void MRTwindow::saveConfig() {

  localSettings.setValue("motor1", ui->axis1->motor->motor()->getPv());
  localSettings.setValue("motor1Start", ui->axis1->ui->start->value());
  localSettings.setValue("motor1End", ui->axis1->ui->end->value());
  localSettings.setValue("motor1Points", ui->axis1->ui->points->value());
  localSettings.setValue("motor1Mode", ui->axis1->ui->mode->currentText());
  localSettings.setValue("2D", ui->use2nd->isChecked());
  localSettings.setValue("motor2", ui->axis2->motor->motor()->getPv());
  localSettings.setValue("motor2Start", ui->axis2->ui->start->value());
  localSettings.setValue("motor2End", ui->axis2->ui->end->value());
  localSettings.setValue("motor2Points", ui->axis2->ui->points->value());
  localSettings.setValue("motor2Mode", ui->axis2->ui->mode->currentText());
  localSettings.setValue("afterScan", ui->after->currentText());

}


void MRTwindow::loadConfig() {

  if ( localSettings.contains("motor1") )
    ui->axis1->motor->motor()->setPv(localSettings.value("motor1").toString());
  if ( localSettings.contains("motor1Start") ) {
    bool ok;
    double val = localSettings.value("motor1Start").toDouble(&ok);
    if (ok) {
      ui->axis1->ui->start->setValue(val);
      ui->axis1->startEndCh();
    }
  }
  if ( localSettings.contains("motor1End") ) {
    bool ok;
    double val = localSettings.value("motor1End").toDouble(&ok);
    if (ok) {
      ui->axis1->ui->end->setValue(val);
      ui->axis1->startEndCh();
    }
  }
  if ( localSettings.contains("motor1Points") ) {
    bool ok;
    int val = localSettings.value("motor1Points").toInt(&ok);
    if (ok) {
      ui->axis1->ui->points->setValue(val);
      ui->axis1->pointsCh(val);
    }
  }
  if ( localSettings.contains("motor1Mode") )
    ui->axis1->ui->mode->setCurrentIndex(
        ui->axis1->ui->mode->findText(
            localSettings.value("motor1Mode").toString() ) );

  if ( localSettings.contains("2D") )
    ui->use2nd->setChecked(localSettings.value("2D").toBool());

  if ( localSettings.contains("motor2") )
    ui->axis2->motor->motor()->setPv(localSettings.value("motor2").toString());
  if ( localSettings.contains("motor2Start") ) {
    bool ok;
    double val = localSettings.value("motor2Start").toDouble(&ok);
    if (ok) {
      ui->axis2->ui->start->setValue(val);
      ui->axis2->startEndCh();
    }
  }
  if ( localSettings.contains("motor2End") ) {
    bool ok;
    double val = localSettings.value("motor2End").toDouble(&ok);
    if (ok) {
      ui->axis2->ui->end->setValue(val);
      ui->axis2->startEndCh();
    }
  }
  if ( localSettings.contains("motor2Points") ) {
    bool ok;
    int val = localSettings.value("motor2Points").toInt(&ok);
    if (ok) {
      ui->axis2->ui->points->setValue(val);
      ui->axis2->pointsCh(val);
    }
  }
  if ( localSettings.contains("motor2Mode") )
    ui->axis2->ui->mode->setCurrentIndex(
        ui->axis2->ui->mode->findText(
            localSettings.value("motor2Mode").toString() ) );

  if ( localSettings.contains("afterScan") )
      ui->after->setCurrentIndex(
          ui->after->findText(
              localSettings.value("afterScan").toString() ) );

}
