#include "mrtwindow.h"
#include <imbl/mrtShutter.h>
#include "ui_mrtwindow.h"





QSettings MRTwindow::localSettings(QDir::homePath() + "/.mrtgui", QSettings::IniFormat);

MRTwindow::MRTwindow(QWidget *parent) :
  QMainWindow(parent),
  shutfast(new GeneralFastShutter),
  shut1A(new Shutter1A),
  ui(new Ui::MRTwindow),
  procBefore(new QProcess),
  procAfter(new QProcess),
  stopme(false)
{
  ui->setupUi(this);

  ui->axis2->setVisible(ui->use2nd->isChecked());

  ui->placeShutter->addWidget(shutfast);

  connect(shutfast->component(), SIGNAL(progressChanged(int)), SLOT(updateShutterStatus()));
  connect(shutfast->component(), SIGNAL(canStartChanged(bool)), SLOT(updateShutterStatus()));
  connect(shutfast->component(), SIGNAL(valuesOKchanged(bool)), SLOT(updateShutterStatus()));
  connect(shutfast->component(), SIGNAL(connectionChanged(bool)), SLOT(updateShutterStatus()));
  connect(ui->axis1->motor->motor(), SIGNAL(changedMoving(bool)), SLOT(updateStartStatus()));
  connect(ui->axis2->motor->motor(), SIGNAL(changedMoving(bool)), SLOT(updateStartStatus()));

  connect(ui->start, SIGNAL(clicked()), SLOT(onStartStop()));

  loadConfig();
  connect(ui->axis1, SIGNAL(settingChanged()), SLOT(saveConfig()));
  connect(ui->axis2, SIGNAL(settingChanged()), SLOT(saveConfig()));
  connect(ui->use2nd, SIGNAL(toggled(bool)), SLOT(saveConfig()));
  connect(ui->after, SIGNAL(activated(int)), SLOT(saveConfig()));

  connect(ui->commandBefore, SIGNAL(textChanged(QString)), SLOT(onChangedBefore()));
  connect(ui->browseBefore, SIGNAL(clicked()), SLOT(onBrowseBefore()));
  connect(ui->execBefore, SIGNAL(clicked()), SLOT(onExecBefore()));
  connect(ui->commandAfter, SIGNAL(textChanged(QString)), SLOT(onChangedAfter()));
  connect(ui->browseAfter, SIGNAL(clicked()), SLOT(onBrowseAfter()));
  connect(ui->execAfter, SIGNAL(clicked()), SLOT(onExecAfter()));

  ui->progressBar->setHidden(true);

  onChangedBefore();
  onChangedAfter();

  //shut->component()->setRepeats(1);

}

MRTwindow::~MRTwindow() {
  delete ui;
  delete shutfast;
  delete shut1A;
}


void MRTwindow::updateShutterStatus() {

  if ( ! shutfast->component()->isConnected() ) {
    ui->disabledShutter->setVisible(false);
    ui->badShutter->setVisible(false);
  } else {
    ui->disabledShutter->setVisible( ! shutfast->component()->canStart() );
    ui->badShutter->setVisible( ! shutfast->component()->valuesOK() );
  }
  updateStartStatus();

}


void MRTwindow::onStartStop() {

  if ( ui->progressBar->isVisible() ) { // in prog
    stopme = true;
    ui->start->setText("Stoppping...");
    updateStartStatus();
    procBefore->kill();
    procAfter->kill();
    shutfast->component()->stop();
    ui->axis1->motor->motor()->stop();
    if(ui->use2nd->isChecked())
      ui->axis2->motor->motor()->stop();
    return;
  }

  // get initial positions
  const int points1 = ui->axis1->points();
  const int points2 = ui->use2nd->isChecked() ? ui->axis2->points() : 1;
  const double initPos1 = ui->axis1->motor->motor()->getUserPosition();
  const double initPos2 = ui->use2nd->isChecked() ?
                          ui->axis2->motor->motor()->getUserPosition()  :  0 ;


  if ( shutfast->component()->exposureMode() != MrtShutter::SOFT ) {
    shutfast->component()->setExposureMode(MrtShutter::SOFT);
    qtWait(shutfast->component(), SIGNAL(exposureModeChanged(MrtShutter::ExposureMode)), 1100);
  }
  if ( shutfast->component()->repeats() != points1 * points2 ) {
    shutfast->component()->setRepeats(points1 * points2);
    qtWait(shutfast->component(), SIGNAL(repeatsChanged(int)), 1100);
  }
  shutfast->component()->start(true);
  qtWait(shutfast->component(), SIGNAL(progressChanged(int)), 1100);
  if ( ! shutfast->component()->progress() )
    qtWait(shutfast->component(), SIGNAL(progressChanged(int)), 2100);
  qtWait(2000);
  if ( shutfast->component()->exposureMode() != MrtShutter::SOFT ||
       shutfast->component()->repeats() != points1 * points2 ||
       ! shutfast->component()->progress() ) {
    qDebug() << "Could not prepare the MRT shutter."
             << shutfast->component()->progress()
             << shutfast->component()->exposureMode()
             << shutfast->component()->repeats();
    shutfast->component()->stop();
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
  updateStartStatus();
  ui->control->setEnabled(false);
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

      if (stopme)
        break;

      if ( ! shutfast->component()->progress() ) {
        qDebug() << "STOPEED AT" << pnt1 << pnt2;
        shutfast->component()->start(true);
        qtWait(shutfast->component(), SIGNAL(progressChanged(int)), 5000);
        int count = 0;
        while ( ! stopme && count++ < 10  && ! shutfast->component()->progress() ) {
          qDebug() << "COULD NOT RESTART. ATTEMPT" << count << " of 10. Will retry in " << count << "seconds.";
          qtWait( count * 1000 ); // wait
          shutfast->component()->start(true);
          qtWait(shutfast->component(), SIGNAL(progressChanged(int)), 5000);
        }
        if (stopme)
          break;
        if ( ! shutfast->component()->progress() ) {
          qDebug() << "FAILED AFTER 10 ATTEMPTS. STOPPING EXPERIMENT. SORRY.";
          onStartStop();
        }
      }
      if (stopme)
        break;

      //int curProg = shut->component()->progress();
      if (procBefore->pid()) { // should never happen
        procBefore->kill();
        qtWait(100);
      }
      onExecBefore();
      if (stopme)
        break;

      shutfast->component()->trig(true);

      if (stopme)
        break;
      if (procAfter->pid()) { // should never happen
        procAfter->kill();
        qtWait(100);
      }
      onExecAfter();
      //if ( shut->component()->progress() != curProg + 1 )

      ui->progressBar->setValue(++curpoint);

      if (stopme)
        break;

    }

    if (stopme)
      break;

  }

  shutfast->component()->stop();
  shut1A->close(false);
  ui->start->setText("Finishing...");
  stopme=true;
  updateStartStatus();

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

  ui->control->setEnabled(true);
  ui->start->setText("Start");
  ui->progressBar->setVisible(false);
  stopme=false;
  updateStartStatus();

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
  localSettings.setValue("beforeExec", ui->commandBefore->text());
  localSettings.setValue("afterExec", ui->commandAfter->text());

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

  if ( localSettings.contains("beforeExec") )
    ui->commandBefore->setText(localSettings.value("beforeExec").toString());
  if ( localSettings.contains("afterExec") )
    ui->commandAfter->setText(localSettings.value("afterExec").toString());

}


void MRTwindow::onBrowseBefore(){
  QString execFile = QFileDialog::getOpenFileName();
  ui->commandBefore->setText(execFile);
}

void MRTwindow::onBrowseAfter() {
  QString execFile = QFileDialog::getOpenFileName();
  ui->commandAfter->setText(execFile);
}

void MRTwindow::onChangedBefore() {
  ui->commandBefore->setStyleSheet("");
  ui->execBefore->setDisabled(
        ui->commandBefore->text().isEmpty() );
  saveConfig();
}


void MRTwindow::onChangedAfter() {
  ui->commandAfter->setStyleSheet("");
  ui->execAfter->setDisabled(
        ui->commandAfter->text().isEmpty() );
  saveConfig();
}

void MRTwindow::onExecBefore() {

  if (procBefore->pid()) { // running
    procBefore->kill();
  } else if ( ! ui->commandBefore->text().isEmpty() ) {

    ui->commandBefore->setStyleSheet("");
    ui->commandBefore->setEnabled(false);
    ui->browseBefore->setEnabled(false);
    ui->execBefore->setText("Stop");

    procBefore->start( ui->commandBefore->text() );
    updateStartStatus();
    while (procBefore->pid())
      qtWait(procBefore, SIGNAL(stateChanged(QProcess::ProcessState)));
    updateStartStatus();
    qDebug() << procBefore->readAll();
    if ( procBefore->exitCode() )
      ui->commandBefore->setStyleSheet("color: rgba(255, 0, 0);");

    ui->execBefore->setText("Exec");
    ui->commandBefore->setEnabled(true);
    ui->browseBefore->setEnabled(true);

  }

}

void MRTwindow::onExecAfter() {

  if (procAfter->pid()) { // running
    procAfter->kill();
  } else if ( ! ui->commandAfter->text().isEmpty() ) {

    ui->commandAfter->setStyleSheet("");
    ui->commandAfter->setEnabled(false);
    ui->browseAfter->setEnabled(false);
    ui->execAfter->setText("Stop");

    procAfter->start( ui->commandAfter->text() );
    updateStartStatus();
    while (procAfter->pid())
      qtWait(procAfter, SIGNAL(stateChanged(QProcess::ProcessState)));
    updateStartStatus();
    qDebug() << procAfter->readAll();
    if ( procAfter->exitCode() )
      ui->commandAfter->setStyleSheet("color: rgba(255, 0, 0, 128);");

    ui->execAfter->setText("Exec");
    ui->commandAfter->setEnabled(true);
    ui->browseAfter->setEnabled(true);

  }

}

void MRTwindow::updateStartStatus() {
  bool enabled;
  if ( ! shutfast->component()->isConnected() ) {
    enabled = false;
  } else if ( ui->progressBar->isVisible() ) {
    enabled = ! stopme;
  } else {
    enabled = shutfast->component()->canStart() && shutfast->component()->valuesOK() &&
             ! procBefore->pid()  &&  ! procAfter->pid() &&
             ui->axis1->motor->motor()->isConnected() &&
             ! ui->axis1->motor->motor()->isMoving();
    if ( enabled && ui->use2nd->isChecked() )
      enabled = ui->axis2->motor->motor()->isConnected() &&
                ! ui->axis2->motor->motor()->isMoving();
  }
  ui->start->setEnabled(enabled);
}
