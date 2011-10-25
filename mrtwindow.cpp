#include "mrtwindow.h"
#include <imbl/mrtShutter.h>
#include "ui_mrtwindow.h"

MRTwindow::MRTwindow(QWidget *parent) :
  QMainWindow(parent),
  shut(new MrtShutter),
  ui(new Ui::MRTwindow)
{
    ui->setupUi(this);
    ui->axis2->setVisible(ui->use2nd->isChecked());


    connect(shut, SIGNAL(canStartChanged(bool)), ui->disabledShutter, SLOT(setVisible(bool)));
    connect(shut, SIGNAL(valuesOKchanged(bool)), ui->badShutter, SLOT(setHidden(bool)));

    connect(shut, SIGNAL(connectionChanged(bool)), SLOT(updateShutterStatus()));
    connect(shut, SIGNAL(stateChanged(MrtShutter::State)), SLOT(updateShutterStatus()));
    connect(shut, SIGNAL(exposureChanged(int)), ui->exposure, SLOT(setValue(int)));

    connect(ui->exposure, SIGNAL(valueEdited(int)), SLOT(setExposure(int)));
    connect(ui->trigShutter, SIGNAL(clicked()), SLOT(trigShutter()));

    ui->progressBar->setHidden(true);

    shut->setRepeats(1);

}

MRTwindow::~MRTwindow()
{
  delete ui;
  delete shut;
}


void MRTwindow::updateShutterStatus() {
  ui->shutLayout->setEnabled(shut->isConnected());
  if ( ! shut->isConnected() ) {
    ui->trigShutter->setText("Disconnected");
    ui->shutterStatus->setText("Disconnected");
  } else {
    switch (shut->state()) {
    case MrtShutter::CLOSED :
      ui->trigShutter->setEnabled(true);
      ui->trigShutter->setText("Open");
      ui->shutterStatus->setText("closed");
      break;
    case MrtShutter::OPENED :
      ui->trigShutter->setEnabled(true);
      ui->trigShutter->setText("Close");
      ui->shutterStatus->setText("opened");
      break;
    case MrtShutter::BETWEEN :
      ui->trigShutter->setDisabled(true);
      ui->trigShutter->setText("N/A");
      ui->shutterStatus->setText("in between");
      break;
    }
  }
}

void MRTwindow::setExposure(int exp) {
  shut->setExposure(exp);
  shut->setCycle(2*exp);
}

void MRTwindow::trigShutter() {
  if (shut->state() == MrtShutter::CLOSED)
    shut->open();
  else if (shut->state() == MrtShutter::OPENED)
    shut->close();
}


void MRTwindow::onStartStop() {

  if (ui->progressBar->isVisible()) { // in prog
    stopme = true;
    return;
  }

  shut->setRepeats(1);

  // get initial positions
  const int points1 = ui->axis1->points();
  const int points2 = ui->use2nd->isChecked() ? ui->axis2->points() : 1;
  const double initPos1 = ui->axis1->motor->motor()->getUserPosition();
  const double initPos2 = ui->use2nd->isChecked() ?
        ui->axis2->motor->motor()->getUserPosition()  :  0 ;

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
  ui->start->setText("Stop");
  ui->progressBar->setValue(0);
  ui->progressBar->setMaximum(points1*points2);
  ui->progressBar->setVisible(true);

  int curpoint = 0;
  for( int pnt2 = 0 ; pnt2 < points2 ; pnt2++ ) {

    if(ui->use2nd->isChecked())
      ui->axis2->motor->motor()->goUserPosition(
            start2 + ( pnt2 * ( end2 - start2 ) ) / (points2 - 1), true);

    if (stopme)
      break;

    for( int pnt1 = 0 ; pnt1 < points1 ; pnt1++ ) {

      ui->axis1->motor->motor()->goUserPosition(
            start1 + ( pnt1 * ( end1 - start1 ) ) / (points1 - 1), true);

      if (stopme)
        break;

      shut->start();
      while ( shut->progress() < 1 )
        qtWait(shut, SIGNAL(progressChanged(int)));

      ui->progressBar->setValue(++curpoint);

      if (stopme)
        break;


    }

    if (stopme)
      break;

  }


  // after scan positioning
  if ( ui->after->currentText() == "Start position" ) {
    ui->axis1->motor->motor()->goUserPosition(start1, true);
    if ( ui->use2nd->isChecked() )
      ui->axis2->motor->motor()->goUserPosition(start2);
  } else if ( ui->after->currentText() == "Prior position" ) {
    ui->axis1->motor->motor()->goUserPosition(initPos1, true);
    if ( ui->use2nd->isChecked() )
      ui->axis2->motor->motor()->goUserPosition(initPos2);
  }


  ui->start->setText("Start");
  ui->progressBar->setVisible(false);


}
