#include "AntonovCNC.h"
#include "ui_AntonovCNC.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <random>

AntonovCNC::AntonovCNC(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::AntonovCNCClass),
    programProgress(0),
    programRunning(false),
    currentProgramRow(0),
    spindleSpeed(1000),
    feedRate(100),
    feedRateMultiplier(1.0),
    spindleSpeedMultiplier(1.0),
    xValueCurrent(0),
    yValueCurrent(0),
    zValueCurrent(0),
    xValueFinal(10),
    yValueFinal(10),
    zValueFinal(10)
{
    ui->setupUi(this);
    ui->slider_spindle_speed->setRange(0, 100);
    ui->slider_feed_rate->setRange(0, 100);

    connect(ui->slider_spindle_speed, &QSlider::valueChanged, this, &AntonovCNC::handleSpindleSpeedChange);
    connect(ui->slider_feed_rate, &QSlider::valueChanged, this, &AntonovCNC::handleFeedRateChange);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AntonovCNC::updateTime);
    timer->start(1000);

    connect(ui->button_numeration, &QPushButton::clicked, this, &AntonovCNC::handleNumeration);
    connect(ui->button_ostatok, &QPushButton::clicked, this, &AntonovCNC::handleOstatokPut);
    connect(ui->button_mmdyum, &QPushButton::clicked, this, &AntonovCNC::handleMmDyum);
    connect(ui->button_changesk, &QPushButton::clicked, this, &AntonovCNC::handleChangeSK);
    connect(ui->button_priv, &QPushButton::clicked, this, &AntonovCNC::handlePriv);
    connect(ui->button_startkadr, &QPushButton::clicked, this, &AntonovCNC::handleStartKadr);
    connect(ui->button_korrekt, &QPushButton::clicked, this, &AntonovCNC::handleKorrektion);
    connect(ui->button_smesh, &QPushButton::clicked, this, &AntonovCNC::handleSmesh);
    connect(ui->button_selectprog, &QPushButton::clicked, this, &AntonovCNC::loadProgram);
    connect(ui->button_back, &QPushButton::clicked, this, &AntonovCNC::handleBack);
    connect(ui->commandLinkButton_stop, &QPushButton::clicked, this, &AntonovCNC::handleStop);
    connect(ui->commandLinkButton_start, &QPushButton::clicked, this, &AntonovCNC::handleStart);
    connect(ui->commandLinkButton_reset, &QPushButton::clicked, this, &AntonovCNC::handleReset);

    updateCoordinatesDisplay();
}

AntonovCNC::~AntonovCNC() {
    delete ui;
}

void AntonovCNC::updateTime() {
    QDateTime currentTime = QDateTime::currentDateTime();
    ui->label_date->setText(currentTime.toString("dd.MM.yyyy"));
    ui->label_time->setText(currentTime.toString("HH:mm:ss"));
}

void AntonovCNC::handleNumeration() {
    qDebug() << "Кнопка 'Нумерация' нажата";
}

void AntonovCNC::handleOstatokPut() {
    qDebug() << "Кнопка 'Остаток пути' нажата";
}

void AntonovCNC::handleMmDyum() {
    qDebug() << "Кнопка 'мм/дюйм' нажата";
}

void AntonovCNC::handleChangeSK() {
    qDebug() << "Кнопка 'Смена СК' нажата";
}

void AntonovCNC::handlePriv() {
    qDebug() << "Кнопка 'Привязка' нажата";
}

void AntonovCNC::handleStartKadr() {
    qDebug() << "Кнопка 'Запуск с кадра' нажата";
}

void AntonovCNC::handleKorrektion() {
    qDebug() << "Кнопка 'Коррекция инструмента' нажата";
}

void AntonovCNC::handleSmesh() {
    qDebug() << "Кнопка 'Смещение нуля' нажата";
}

void AntonovCNC::handleBack() {
    qDebug() << "Кнопка 'Возврат' нажата";
}

void AntonovCNC::handleStop() {
    if (programRunning) {
        programRunning = false;
        qDebug() << "Программа остановлена";
    }
}

void AntonovCNC::handleStart() {
    if (!programRunning && !loadedProgram.isEmpty()) {
        programRunning = true;
        programProgress = 0;
        currentProgramRow = 0;
        analyzeProgram();
        QTimer::singleShot(100, this, &AntonovCNC::updateProgressBar);
        qDebug() << "Программа запущена";
    }
}

void AntonovCNC::handleReset() {
    programProgress = 0;
    currentProgramRow = 0;
    updateStatusBar(0);
    programRunning = false;

    xValueCurrent = 0;
    yValueCurrent = 0;
    zValueCurrent = 0;
    xValueFinal = 10;
    yValueFinal = 10;
    zValueFinal = 10;

    updateCoordinatesDisplay();

    if (ui->listWidget_program->count() > 0) {
        QListWidgetItem* item = ui->listWidget_program->item(0);
        ui->listWidget_program->setCurrentItem(item);
    }
}

void AntonovCNC::updateProgressBar() {
    if (programRunning && currentProgramRow < loadedProgram.size()) {
        int progressPercentage = static_cast<int>((static_cast<double>(currentProgramRow) / loadedProgram.size()) * 100);
        updateStatusBar(progressPercentage);
        highlightCurrentProgramRow(currentProgramRow);
        currentProgramRow++;
        extractNextCoordinates();
        QTimer::singleShot(1000, this, &AntonovCNC::updateProgressBar);
    }
    else {
        programRunning = false;

        if (ui->listWidget_program->count() > 0) {
            QListWidgetItem* item = ui->listWidget_program->item(0);
            ui->listWidget_program->setCurrentItem(item);
        }
    }
}

void AntonovCNC::loadProgram() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Выбрать программу"), "", tr("Файлы программы (*.txt *.nc);;Все файлы (*.*)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            loadedProgram.clear();
            ui->listWidget_program->clear();

            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                loadedProgram.append(line);
                ui->listWidget_program->addItem(line);
            }

            analyzeProgram();
            qDebug() << "Загружена программа: " << fileName;
        }
    }
}

void AntonovCNC::updateStatusBar(int value) {
    ui->progressBar_runtime->setValue(value);
}

void AntonovCNC::highlightCurrentProgramRow(int row) {
    if (row < ui->listWidget_program->count()) {
        QListWidgetItem* item = ui->listWidget_program->item(row);
        ui->listWidget_program->setCurrentItem(item);
        extractCoordinatesAndSpeed(loadedProgram[row]);
    }
}

void AntonovCNC::handleSpindleSpeedChange() {
    spindleSpeedMultiplier = 1.0 + (ui->slider_spindle_speed->value() / 100.0);
    ui->label_spindle_value->setText(QString::number(spindleSpeed * spindleSpeedMultiplier));
}

void AntonovCNC::handleFeedRateChange() {
    feedRateMultiplier = 1.0 + (ui->slider_feed_rate->value() / 100.0);
    ui->label_feed_value->setText(QString::number(feedRate * feedRateMultiplier));
}

void AntonovCNC::analyzeProgram() {
    QSet<QString> gCodes, mCodes, tCodes, dCodes;

    for (const QString& line : loadedProgram) {
        extractCoordinatesAndSpeed(line);

        QRegularExpression regexG(R"(G\d{1,3})");
        QRegularExpression regexM(R"(M\d{1,3})");
        QRegularExpression regexT(R"(T\d{1,2})");
        QRegularExpression regexD(R"(D\d{1,2})");

        QRegularExpressionMatch matchG = regexG.match(line);
        if (matchG.hasMatch()) {
            gCodes.insert(matchG.captured(0));
        }

        QRegularExpressionMatch matchM = regexM.match(line);
        if (matchM.hasMatch()) {
            mCodes.insert(matchM.captured(0));
        }

        QRegularExpressionMatch matchT = regexT.match(line);
        if (matchT.hasMatch()) {
            tCodes.insert(matchT.captured(0));
        }

        QRegularExpressionMatch matchD = regexD.match(line);
        if (matchD.hasMatch()) {
            dCodes.insert(matchD.captured(0));
        }
    }

    QList<QLabel*> gLabels = { ui->label_g_code, ui->label_g_code_1, ui->label_g_code_2, ui->label_g_code_3 };
    QList<QLabel*> mLabels = { ui->label_m_code, ui->label_m_code_1, ui->label_m_code_2, ui->label_m_code_3 };
    QList<QLabel*> tLabels = { ui->label_t_code, ui->label_t_code_1, ui->label_t_code_2, ui->label_t_code_3 };
    QList<QLabel*> dLabels = { ui->label_d_code, ui->label_d_code_1, ui->label_d_code_2, ui->label_d_code_3 };

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(gLabels.begin(), gLabels.end(), g);
    std::shuffle(mLabels.begin(), mLabels.end(), g);
    std::shuffle(tLabels.begin(), tLabels.end(), g);
    std::shuffle(dLabels.begin(), dLabels.end(), g);

    int index = 0;
    for (const QString& gCode : gCodes) {
        if (index < gLabels.size()) gLabels[index++]->setText(gCode);
    }

    index = 0;
    for (const QString& mCode : mCodes) {
        if (index < mLabels.size()) mLabels[index++]->setText(mCode);
    }

    index = 0;
    for (const QString& tCode : tCodes) {
        if (index < tLabels.size()) tLabels[index++]->setText(tCode);
    }

    index = 0;
    for (const QString& dCode : dCodes) {
        if (index < dLabels.size()) dLabels[index++]->setText(dCode);
    }
}

void AntonovCNC::extractCoordinatesAndSpeed(const QString& line, bool isNext) {
    QRegularExpression regexX(R"(X(-?\d+\.?\d*))");
    QRegularExpression regexY(R"(Y(-?\d+\.?\d*))");
    QRegularExpression regexZ(R"(Z(-?\d+\.?\d*))");
    QRegularExpression regexF(R"(F(\d+))");
    QRegularExpression regexS(R"(S(\d+))");

    QRegularExpressionMatch matchX = regexX.match(line);
    if (matchX.hasMatch()) {
        if (isNext) {
            xValueFinal = matchX.captured(1).toDouble();
        }
        else {
            xValueCurrent = matchX.captured(1).toDouble();
        }
    }

    QRegularExpressionMatch matchY = regexY.match(line);
    if (matchY.hasMatch()) {
        if (isNext) {
            yValueFinal = matchY.captured(1).toDouble();
        }
        else {
            yValueCurrent = matchY.captured(1).toDouble();
        }
    }

    QRegularExpressionMatch matchZ = regexZ.match(line);
    if (matchZ.hasMatch()) {
        if (isNext) {
            zValueFinal = matchZ.captured(1).toDouble();
        }
        else {
            zValueCurrent = matchZ.captured(1).toDouble();
        }
    }

    QRegularExpressionMatch matchF = regexF.match(line);
    if (matchF.hasMatch()) {
        feedRate = matchF.captured(1).toInt();
        ui->label_feed_value->setText(QString::number(feedRate * feedRateMultiplier));
    }

    QRegularExpressionMatch matchS = regexS.match(line);
    if (matchS.hasMatch()) {
        spindleSpeed = matchS.captured(1).toInt();
        ui->label_spindle_value->setText(QString::number(spindleSpeed * spindleSpeedMultiplier));
    }

    updateCoordinatesDisplay();
}

void AntonovCNC::extractNextCoordinates() {
    if (currentProgramRow + 1 < loadedProgram.size()) {
        extractCoordinatesAndSpeed(loadedProgram[currentProgramRow + 1], true);
    }
    else {
        xValueCurrent = xValueFinal;
        yValueCurrent = yValueFinal;
        zValueCurrent = zValueFinal;
    }
}

void AntonovCNC::updateCoordinatesDisplay() {
    ui->label_x_value_current->setText(QString::number(xValueCurrent));
    ui->label_y_value_current->setText(QString::number(yValueCurrent));
    ui->label_z_value_current->setText(QString::number(zValueCurrent));

    ui->label_x_value_final->setText(QString::number(xValueFinal));
    ui->label_y_value_final->setText(QString::number(yValueFinal));
    ui->label_z_value_final->setText(QString::number(zValueFinal));
}
