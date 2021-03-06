#include "bookingdialog.h"
#include "ui_bookingdialog.h"

#define UPDATE_TIME 30000

BookingDialog::BookingDialog(int id, int vMode,  QWidget *parent) :
    idCar(id),
    viewMode(static_cast<ViewMode>(vMode)),
    QDialog(parent),
    ui(new Ui::BookingDialog)
{
    ui->setupUi(this);

    this->setFixedHeight(170);

    setCalendarColor(ui->calendarWidget,QColor(255,140,0));
    setCalendarColor(ui->dateTimeEditBegin->calendarWidget(),QColor(255,140,0));
    setCalendarColor(ui->dateTimeEditEnd->calendarWidget(),QColor(255,140,0));
    ui->dateTimeEditBegin->setDateTime(QDateTime::currentDateTime());
    ui->dateTimeEditEnd->setDateTime(QDateTime::currentDateTime());

    carReservations = new QSqlQueryModel(this);
    statusHistory = new QSqlQueryModel(this);

    QSqlQueryModel * windTitle = new QSqlQueryModel(this);
    windTitle->setQuery(QString("SELECT Brand, Model, LicensePlate FROM car WHERE idCar = %1").arg(idCar));
    this->setWindowTitle( QString("Rezerwacja - ")
                          + windTitle->data(windTitle->index(windTitle->rowCount()-1,0)).toString() + QString(" ")
                          + windTitle->data(windTitle->index(windTitle->rowCount()-1,1)).toString() + QString(" - ")
                          + windTitle->data(windTitle->index(windTitle->rowCount()-1,2)).toString()
                          );
    delete windTitle;

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(onTimerOverflow()));
    connect(this, SIGNAL(bookedCar()), this, SLOT(updateView()),Qt::QueuedConnection);
    onTimerOverflow();

    isOpen = true;
}

bool BookingDialog::isOpen;

BookingDialog::~BookingDialog()
{
    firstInit = true;
    isOpen = false;
    delete ui;
}

void BookingDialog::updateView()
{
    //qDebug() << "Updating booking ..." << endl;
    if(firstInit) {
        ui->calendarWidget->clicked(QDate::currentDate());
        firstInit=false;
    }
    else
        on_calendarWidget_clicked(choosenDate);
}

void BookingDialog::onTimerOverflow()
{
    updateView();
    timer->start(UPDATE_TIME);
}

void BookingDialog::fillCalendar()
{
    QTextCharFormat format;
    QDate itDate;

    clearCalendarFormat();

    if(viewMode == ViewMode::Booked) {

        format.setBackground(QBrush(QColor(0,186,18), Qt::SolidPattern));

        for(int i = 0; i < carReservations->rowCount(); ++i) {

            itDate = carReservations->data(carReservations->index(i,3)).toDate();

            while(itDate <= carReservations->data(carReservations->index(i,4)).toDate()) {
                ui->calendarWidget->setDateTextFormat(itDate, format);
                itDate = itDate.addDays(1);
            }
        }
    }

    else if(viewMode == ViewMode::History) {

        format.setBackground(QBrush(QColor(80,90,210), Qt::SolidPattern));
        format.setForeground(QBrush(QColor(255,255,255), Qt::SolidPattern));

        for(int i = 0; i < statusHistory->rowCount(); ++i) {

            itDate = statusHistory->data(statusHistory->index(i,3)).toDate();

            while(itDate <= statusHistory->data(statusHistory->index(i,4)).toDate()) {
                ui->calendarWidget->setDateTextFormat(itDate, format);
                itDate = itDate.addDays(1);
            }
        }
    }
}

void BookingDialog::loadBlock(int idx)
{
    QDate beginDate;
    QDate endDate;

    if(viewMode == ViewMode::Booked) {

        beginDate = carReservations->data(carReservations->index(idx,3)).toDate();
        endDate = carReservations->data(carReservations->index(idx,4)).toDate();

        if(choosenDate >= beginDate && choosenDate <= endDate) {

            if(beginDate != endDate) {

                if(choosenDate > beginDate && choosenDate < endDate)
                    blockVector.emplace_back(std::move(new BookingBlock(carReservations->data(carReservations->index(idx,0)).toInt(),carReservations->data(carReservations->index(idx,1)).toString() + QString(" ") + carReservations->data(carReservations->index(idx,2)).toString(),
                                                                        carReservations->data(carReservations->index(idx,6)).toString() ,QString("..."), QString("..."), false)));
                else if (choosenDate == beginDate)
                    blockVector.emplace_back(std::move(new BookingBlock(carReservations->data(carReservations->index(idx,0)).toInt(),carReservations->data(carReservations->index(idx,1)).toString() + QString(" ") + carReservations->data(carReservations->index(idx,2)).toString(),
                                                                        carReservations->data(carReservations->index(idx,6)).toString() ,carReservations->data(carReservations->index(idx,3)).toDateTime().time().toString("hh:mm"), QString("..."))));
                else if(choosenDate == endDate)
                    blockVector.emplace_back(std::move(new BookingBlock(carReservations->data(carReservations->index(idx,0)).toInt(),carReservations->data(carReservations->index(idx,1)).toString() + QString(" ") + carReservations->data(carReservations->index(idx,2)).toString(),
                                                                        carReservations->data(carReservations->index(idx,6)).toString() ,QString("..."), carReservations->data(carReservations->index(idx,4)).toDateTime().time().toString("hh:mm"))));
            }
            else
                blockVector.emplace_back(std::move(new BookingBlock(carReservations->data(carReservations->index(idx,0)).toInt(),carReservations->data(carReservations->index(idx,1)).toString() + QString(" ") + carReservations->data(carReservations->index(idx,2)).toString(),
                                                                    carReservations->data(carReservations->index(idx,6)).toString() , carReservations->data(carReservations->index(idx,3)).toDateTime().time().toString("hh:mm"), carReservations->data(carReservations->index(idx,4)).toDateTime().time().toString("hh:mm"))));

            connect(blockVector.back(),SIGNAL(refresh()),this,SLOT(updateView()),Qt::QueuedConnection);
            connect(blockVector.back(),&BookingBlock::inProgress,this,[=](){ timer->stop();});
            connect(blockVector.back(),&BookingBlock::progressFinished, this, [=](){timer->start(UPDATE_TIME);});
        }
    }
    else if(viewMode == ViewMode::History) {

        beginDate = statusHistory->data(statusHistory->index(idx,3)).toDate();
        endDate = statusHistory->data(statusHistory->index(idx,4)).toDate();

        if(choosenDate >= beginDate && choosenDate <= endDate) {

            if(beginDate != endDate) {

                if(choosenDate > beginDate && choosenDate < endDate)
                    blockVector.emplace_back(std::move(new BookingBlock(-1,statusHistory->data(statusHistory->index(idx,1)).toString() + QString(" ") + statusHistory->data(statusHistory->index(idx,2)).toString(),
                                                                        statusHistory->data(statusHistory->index(idx,6)).toString(),QString("..."), QString("..."), false, false)));
                else if (choosenDate == beginDate)
                    blockVector.emplace_back(std::move(new BookingBlock(-1,statusHistory->data(statusHistory->index(idx,1)).toString() + QString(" ") + statusHistory->data(statusHistory->index(idx,2)).toString(),
                                                                        statusHistory->data(statusHistory->index(idx,6)).toString() ,statusHistory->data(statusHistory->index(idx,3)).toDateTime().time().toString("hh:mm"), QString("..."), true, false)));
                else if(choosenDate == endDate)
                    blockVector.emplace_back(std::move(new BookingBlock(-1,statusHistory->data(statusHistory->index(idx,1)).toString() + QString(" ") + statusHistory->data(statusHistory->index(idx,2)).toString(),
                                                                        statusHistory->data(statusHistory->index(idx,6)).toString(),QString("..."), statusHistory->data(statusHistory->index(idx,4)).toDateTime().time().toString("hh:mm"), true, false)));
            }
            else
                blockVector.emplace_back(std::move(new BookingBlock(-1,statusHistory->data(statusHistory->index(idx,1)).toString() + QString(" ") + statusHistory->data(statusHistory->index(idx,2)).toString(),
                                                                    statusHistory->data(statusHistory->index(idx,6)).toString(),statusHistory->data(statusHistory->index(idx,3)).toDateTime().time().toString("hh:mm"), statusHistory->data(statusHistory->index(idx,4)).toDateTime().time().toString("hh:mm"), true, false)));
        }
    }
}

void BookingDialog::setCalendarColor(QCalendarWidget *calendarWidget,QColor color)
{
    QWidget *calendarNavBar = calendarWidget->findChild<QWidget *>("qt_calendar_navigationbar");
    if (calendarNavBar) {
        QPalette pal = calendarNavBar->palette();
        pal.setColor(calendarNavBar->backgroundRole(), color);
        calendarNavBar->setPalette(pal);
    }
}

void BookingDialog::on_calendarWidget_clicked(const QDate &date)
{
    if(Database::isOpen()) {

        carReservations->setQuery(QString("SELECT * FROM booking WHERE idCar = %1").arg(idCar));
        statusHistory->setQuery(QString("SELECT * FROM history WHERE idCar = %1").arg(idCar));
        int varticalPosition = ui->scrollArea->verticalScrollBar()->value(); // previous scrollBar position

        choosenDate = date;

        blockVector.clear();
        clearScrollArea();

        if(viewMode == ViewMode::Booked) {
            for(int i = 0; i < carReservations->rowCount(); ++i)
                loadBlock(i);
        }
        else if(viewMode == ViewMode::History) {
            for(int i = 0; i < statusHistory->rowCount(); ++i)
                loadBlock(i);
        }

        std::sort(blockVector.begin(),blockVector.end(),PointerCompare());

        for(auto pos = blockVector.begin(); pos != blockVector.end(); ++pos)
            scrollLayout->addWidget(*pos);
        ui->scrollArea->setWidget(scrollWidget);
        ui->scrollArea->verticalScrollBar()->setValue(varticalPosition);

        fillCalendar();
    }
    else QMessageBox::critical(this,tr("Błąd!"), tr("Utracono połączenie z bazą danych!"));
}

void BookingDialog::clearScrollArea()
{
    delete scrollLayout;
    delete scrollWidget;
    scrollWidget = new QWidget(ui->scrollArea);
    scrollLayout = new QVBoxLayout(scrollWidget);
}

bool BookingDialog::isDateFree()
{
    QSqlQueryModel * bookedDates = new QSqlQueryModel(this);
    bookedDates->setQuery(QString("SELECT Begin, End FROM booking WHERE idCar = %1").arg(idCar));

    QDateTime modelBegin = QDateTime::currentDateTime(), modelEnd = QDateTime::currentDateTime();

    if(ui->dateTimeEditBegin->dateTime().addSecs(60) < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, tr("Uwaga!"), tr("Data i czas początku rezerwacji muszą być większe od aktualnej."));
        return false;
    }

    if(ui->dateTimeEditEnd->dateTime() <= ui->dateTimeEditBegin->dateTime().addSecs(60)) {
        QMessageBox::warning(this, tr("Uwaga!"), tr("Data i czas końca rezerwacji muszą być większe od początkowej."));
        return false;
    }

    for(int i = 0; i < bookedDates->rowCount(); ++i) {

        modelBegin = bookedDates->data(bookedDates->index(i,0)).toDateTime();
        modelEnd = bookedDates->data(bookedDates->index(i,1)).toDateTime();

        if(ui->dateTimeEditBegin->dateTime() >= modelBegin && ui->dateTimeEditEnd->dateTime() <= modelEnd) {
            QMessageBox::warning(this, tr("Uwaga!"), tr("Termin nie może być zarezerwowany."));
            return false;
        }

        if(ui->dateTimeEditBegin->dateTime() <= modelBegin && ui->dateTimeEditEnd->dateTime() >= modelEnd) {
            QMessageBox::warning(this, tr("Uwaga!"), tr("Termin nie może być zarezerwowany."));
            return false;
        }

        if(ui->dateTimeEditBegin->dateTime()<= modelBegin && ui->dateTimeEditEnd->dateTime() >= modelBegin && ui->dateTimeEditEnd->dateTime() <= modelEnd) {
            QMessageBox::warning(this, tr("Uwaga!"), tr("Termin nie może być zarezerwowany."));
            return false;
        }

        if(ui->dateTimeEditBegin->dateTime() >= modelBegin && ui->dateTimeEditBegin->dateTime() <= modelEnd && ui->dateTimeEditEnd->dateTime() >= modelEnd) {
            QMessageBox::warning(this, tr("Uwaga!"), tr("Termin nie może być zarezerwowany."));
            return false;
        }
    }
    return true;
}

void BookingDialog::on_btnReserve_clicked()
{
    if(Database::isOpen()) {

        if(isDateFree()) {
            NameDialog n;
            timer->stop();
            if(n.exec() == NameDialog::Accepted) {
                if(isDateFree()) {
                    QSqlQuery qry;
                    qry.prepare("INSERT INTO booking(Name,Surname,Begin,End,idCar,Destination)"
                             "VALUES(:_Name,:_Surname,:_Begin,:_End,:_idCar,:_Destination)");

                    qry.bindValue(":_Name", n.getName());
                    qry.bindValue(":_Surname", n.getSurname());
                    qry.bindValue(":_Begin", ui->dateTimeEditBegin->dateTime());
                    qry.bindValue(":_End", ui->dateTimeEditEnd->dateTime());
                    qry.bindValue(":_idCar",idCar);
                    qry.bindValue(":_Destination",n.getDestination());
                    if(!qry.exec())
                        QMessageBox::warning(this,tr("Uwaga!"),"Dodawanie nie powiodło się.\nERROR: "+qry.lastError().text()+"");
                    else
                        QMessageBox::information(this,tr("Informacja"),tr("Dodano rezerwację!"));
                }

                timer->start(UPDATE_TIME);
                emit bookedCar();
            }
        }       
    }
    else QMessageBox::critical(this,tr("Błąd!"), tr("Utracono połączenie z bazą danych!"));

}

void BookingDialog::clearCalendarFormat()
{
    QTextCharFormat format;
    format.setBackground(QBrush(QColor(255,255,255,255), Qt::SolidPattern));

    for(auto itr : ui->calendarWidget->dateTextFormat().keys())
        ui->calendarWidget->setDateTextFormat(itr, format);
}

void BookingDialog::on_btnShowHistory_clicked()
{
    if(Database::isOpen()) {
        ui->lblCheck->setGeometry(239,187,21,18);
        viewMode = ViewMode::History;
        updateView();
    }
    else QMessageBox::critical(this,tr("Błąd!"), tr("Utracono połączenie z bazą danych!"));
}

void BookingDialog::on_btnShowReservation_clicked()
{
    if(Database::isOpen()) {
        ui->lblCheck->setGeometry(13,187,21,18);
        viewMode = ViewMode::Booked;
        updateView();
    }
    else QMessageBox::critical(this,tr("Błąd!"), tr("Utracono połączenie z bazą danych!"));
}

void BookingDialog::on_pushButton_clicked()
{
    static bool isExpanded = false;
    isExpanded = !isExpanded;

    if(isExpanded) {
        this->setFixedHeight(500);
        ui->pushButton->setIcon(QIcon(":/images/images/upArrow.png"));
    }

    else {
        this->setFixedHeight(170);
        ui->pushButton->setIcon(QIcon(":/images/images/downArrow.png"));
    }
}
