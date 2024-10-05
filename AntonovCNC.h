#ifndef ANTONOVCNC_H
#define ANTONOVCNC_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTimer>
#include <QLabel>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui { class AntonovCNCClass; }
QT_END_NAMESPACE

class AntonovCNC : public QMainWindow {
    Q_OBJECT

public:
    AntonovCNC(QWidget* parent = nullptr);
    ~AntonovCNC();

private slots:
    void updateTime();
    void handleNumeration();
    void handleOstatokPut();
    void handleMmDyum();
    void handleChangeSK();
    void handlePriv();
    void handleStartKadr();
    void handleKorrektion();
    void handleSmesh();
    void handleBack();
    void handleStop();
    void handleStart();
    void handleReset();
    void updateProgressBar();
    void loadProgram();
    void handleSpindleSpeedChange();
    void handleFeedRateChange();

private:
    Ui::AntonovCNCClass* ui;
    QTimer* timer;
    int programProgress;
    bool programRunning;
    int currentProgramRow;
    int spindleSpeed;
    int feedRate;
    double feedRateMultiplier;
    double spindleSpeedMultiplier;
    double xValueCurrent;
    double yValueCurrent;
    double zValueCurrent;
    double xValueFinal;
    double yValueFinal;
    double zValueFinal;
    QStringList loadedProgram;

    void updateStatusBar(int value);
    void highlightCurrentProgramRow(int row);
    void analyzeProgram();
    void extractCoordinatesAndSpeed(const QString& line, bool isNext = false);
    void updateCoordinatesDisplay();
    void extractNextCoordinates();
    void displayActiveFunctionsRandomly();
    void clearActiveFunctions();

    QList<QLabel*> activeFunctionLabels;
};

#endif // ANTONOVCNC_H
