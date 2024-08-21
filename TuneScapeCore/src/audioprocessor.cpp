#include "include/audioprocessor.h"
#include <QAudioBuffer>
#include <QDebug>
#include <QGraphicsRectItem>
#include <QPainter>
#include <QtConcurrent>
#include <algorithm>

AudioProcessor::AudioProcessor()
    : audioDecoder(nullptr)
    , fftState(nullptr)
    , paintingEnabled(true)
{
    audioDecoder = new QAudioDecoder(this);

    inputBuffer.resize(FFT_BUFFER_SIZE);
    outputBuffer.resize(FFT_BUFFER_SIZE / 2 + 1);
    fftState = fft.init();
}

AudioProcessor::~AudioProcessor()
{
    fft.close(fftState);
    delete audioDecoder;
}

void AudioProcessor::start(const QString &filePath)
{
    audioDecoder->setSource(QUrl::fromLocalFile(filePath));
    audioDecoder->start();
    setPaintingEnabled(true);
    setShouldAutoRestart(true);
    if (updateTimer && updateTimer->isActive()) {
        updateTimer->stop();
    }
    initTimer();
}

void AudioProcessor::stop()
{
    audioDecoder->stop();
    setPaintingEnabled(false);
    setShouldAutoRestart(false);
    outputBuffer.clear();
    if (updateTimer && updateTimer->isActive()) {
        updateTimer->stop();
    }
    graphScene->clear();
}

QMediaPlayer *AudioProcessor::getPlayer() const
{
    return player;
}

void AudioProcessor::setPlayer(QMediaPlayer *newPlayer)
{
    this->player = newPlayer;
}

void AudioProcessor::setPaintingEnabled(bool enabled)
{
    paintingEnabled = enabled;
}

// void AudioProcessor::handleBufferReady()
// {
//     QAudioBuffer buffer = audioDecoder->read();
//     qint64 len = buffer.sampleCount();
//     if (len > FFT_BUFFER_SIZE) {
//         len = FFT_BUFFER_SIZE;
//     }

//     const qint16 *data = buffer.constData<qint16>();
//     std::copy(data, data + len, inputBuffer.data());

//     QtConcurrent::run([this]() {
//         fft.perform(inputBuffer, outputBuffer, fftState);

//         QMetaObject::invokeMethod(this, [this]() {
//             if (!graphScene) {
//                 qDebug() << "graphScene is null!";
//                 return;
//             }

//             graphScene->clear();
//             int width = ui->graphicsView->width();
//             int height = ui->graphicsView->height();
//             int barWidth = width / 16;
//             float maxValue = *std::max_element(outputBuffer.begin(), outputBuffer.end());
//             if (maxValue == 0) {
//                 maxValue = 1; // Avoid division by zero
//             }
//             for (int i = 0; i < 16 && i < outputBuffer.size(); ++i) {
//                 float normalizedValue = outputBuffer[i] / maxValue;
//                 int barHeight = static_cast<int>(normalizedValue * height);
//                 QGraphicsRectItem *bar = new QGraphicsRectItem(i * barWidth, height - barHeight, barWidth - 1, barHeight);
//                 bar->setBrush(Qt::green);
//                 graphScene->addItem(bar);
//             }
//         }, Qt::QueuedConnection);
//     });
// }

void AudioProcessor::handleFinished()
{
    qDebug() << "Decoding finished.";

    // audioDecoder->stop();
    // inputBuffer.clear();
    // outputBuffer.clear();

    // if (getShouldAutoRestart()) {
    //     audioDecoder->start();
    // }
}

void AudioProcessor::processAudioData()
{
    fft.perform(inputBuffer, outputBuffer, fftState);
}

void AudioProcessor::updateGraph()
{
    if (!graphScene || !ui)
        return;

    if (!audioDecoder->isDecoding()) {
        qDebug() << "AudioDecoder is not in the decoding state, resetting...";
        qDebug() << "AudioDecoder error occurred:" << audioDecoder->errorString();
        resetAudioDecoder();
        return;
    }

    // Retrieve and process the audio data
    QAudioBuffer buffer = audioDecoder->read();
    if (!buffer.isValid()) {
        qDebug() << "Invalid buffer or no data available.";
        return;
    }

    QtConcurrent::run([this, buffer]() {
        qint64 len = buffer.sampleCount();
        if (len > FFT_BUFFER_SIZE) {
            len = FFT_BUFFER_SIZE;
        }

        const qint16 *data = buffer.constData<qint16>();
        std::copy(data, data + len, inputBuffer.data());

        // Perform FFT on the data
        fft.perform(inputBuffer, outputBuffer, fftState);

        // Calculate the maximum value to normalize the bars
        float maxValue = *std::max_element(outputBuffer.begin(), outputBuffer.end());
        if (maxValue == 0) {
            maxValue = 1; // Avoid division by zero
        }

        QMetaObject::invokeMethod(
            this,
            [this, maxValue]() {
                // Clear the previous content of the scene
                graphScene->clear();

                // Determine the dimensions for the bars
                int width = ui->graphicsView->width();
                int height = ui->graphicsView->height();
                int barWidth = width / 16;

                // Draw the bars in the scene
                for (int i = 0; i < 16 && i < outputBuffer.size(); ++i) {
                    float normalizedValue = outputBuffer[i] / maxValue;
                    int barHeight = static_cast<int>(normalizedValue * height);
                    QGraphicsRectItem *bar = new QGraphicsRectItem(i * barWidth,
                                                                   height - barHeight,
                                                                   barWidth - 1,
                                                                   barHeight);
                    bar->setBrush(Qt::green);
                    graphScene->addItem(bar);
                }

                qDebug() << "Graph updated.";
            },
            Qt::QueuedConnection);
    });
}

void AudioProcessor::resetAudioDecoder()
{
    audioDecoder->stop();

    QUrl currentSource = audioDecoder->source();
    if (currentSource.isEmpty()) {
        qDebug() << "No source set for audioDecoder.";
        return;
    }

    audioDecoder->setSource(currentSource);
    audioDecoder->start();

    qDebug() << "AudioDecoder has been reset.";
}

void AudioProcessor::initTimer()
{
    if (!updateTimer) {
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &AudioProcessor::updateGraph);
    }

    updateTimer->start(100); // Update every 100 ms
}

bool AudioProcessor::getShouldAutoRestart() const
{
    return shouldAutoRestart;
}

void AudioProcessor::setShouldAutoRestart(bool newShouldAutoRestart)
{
    shouldAutoRestart = newShouldAutoRestart;
}

void AudioProcessor::setUi(Ui::MainWindow *newUi)
{
    ui = newUi;
    if (!graphScene) {
        graphScene = new QGraphicsScene(this);
        ui->graphicsView->setScene(graphScene);
        ui->graphicsView->fitInView(graphScene->sceneRect(), Qt::KeepAspectRatio);

        ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}
