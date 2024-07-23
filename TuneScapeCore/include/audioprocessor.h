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

class AudioProcessor : public QWidget
{
    Q_OBJECT

public:
    AudioProcessor(QWidget *parent = nullptr);
    ~AudioProcessor();

    void start(const QString &filePath);
    void stop();

    QMediaPlayer *getPlayer() const;
    void setPlayer(QMediaPlayer *newPlayer);
    void setPaintingEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void handleBufferReady();
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

    bool paintingEnabled;
};

#endif // AUDIOPROCESSOR_H
