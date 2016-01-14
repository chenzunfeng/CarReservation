#include "carblock.h"
#include "ui_carblock.h"

CarBlock::CarBlock(int id, QString name, QString model, QString licensePlate, QDate inspectionDate, QDate insuranceDate, int mileage, QString notes, Status status, QString photoPath, QWidget *parent) :
    idCar(id),
    QWidget(parent),
    ui(new Ui::CarBlock)
{
    ui->setupUi(this);

    ui->lblLicensePlate->setText(licensePlate);
    ui->lblPhoto->setPixmap(QPixmap(photoPath));
    ui->lblCarName->setText(name + QString(" ") + model);
    ui->lblMileage->setText(QString::number(mileage) + " km");
    setStatus(status);
}

CarBlock::~CarBlock()
{
    delete ui;
}

void CarBlock::setStatus(Status status)
{
    if(status == Status::Free)
        ui->lblStatus->setPixmap(QPixmap(":/images/images/free.png"));
    if(status == Status::Rented)
        ui->lblStatus->setPixmap(QPixmap(":/images/images/rented.png"));
}

void CarBlock::on_btnReserve_clicked()
{
    bookingDialog = new BookingDialog(bookingTable, idCar);
    if(bookingDialog->exec() == BookingDialog::Rejected)
        delete bookingDialog;
}
