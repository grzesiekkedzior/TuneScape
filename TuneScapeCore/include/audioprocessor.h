#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QAudioDecoder>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>
#include <QMediaPlayer>
#include <QUrl>
#include <QVector>
#include <QWidget>
#include "fft.h"
#include <QGraphicsScene>
#include "ui_mainwindow.h"
#include <QTimer>

class AudioProcessor : public QObject
{
    Q_OBJECT

public:
    AudioProcessor();
    ~AudioProcessor();

    void start(const QString &filePath);
    void stop();

    QMediaPlayer *getPlayer() const;
    void setPlayer(QMediaPlayer *newPlayer);
    void setPaintingEnabled(bool enabled);

    void setUi(Ui::MainWindow *newUi);

    bool getShouldAutoRestart() const;
    void setShouldAutoRestart(bool newShouldAutoRestart);
    void resetAudioDecoder();
private slots:
    //void handleBufferReady();
    void handleFinished();
    void processAudioData();

private:
    QAudioDecoder *audioDecoder;
    QBuffer audioBuffer;
    FFT fft;
    FFTState *fftState;
    QVector<sound_sample> inputBuffer;
    QVector<float> outputBuffer;
    QMediaPlayer *player;
    Ui::MainWindow *ui = nullptr;
    QGraphicsScene *graphScene = nullptr;

    QTimer *updateTimer = nullptr;
    void updateGraph();

    bool paintingEnabled;
    bool shouldAutoRestart;
    void initTimer();


};

#endif // AUDIOPROCESSOR_H
