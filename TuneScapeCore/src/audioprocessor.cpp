#include "include/audioprocessor.h"
#include <QAudioBuffer>
#include <QDebug>
#include <QPainter>
#include <algorithm>

AudioProcessor::AudioProcessor(QWidget *parent)
    : QWidget(parent)
    , audioDecoder(nullptr)
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
    connect(audioDecoder, &QAudioDecoder::finished, this, &AudioProcessor::handleFinished);
    connect(player, &QMediaPlayer::positionChanged, this, &AudioProcessor::handleBufferReady);
}

void AudioProcessor::stop()
{
    audioDecoder->stop();
    disconnect(audioDecoder, &QAudioDecoder::finished, this, &AudioProcessor::handleFinished);
    disconnect(player, &QMediaPlayer::positionChanged, this, &AudioProcessor::handleBufferReady);
    setPaintingEnabled(false);
    outputBuffer.clear();
    update();
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

void AudioProcessor::handleBufferReady()
{
    QAudioBuffer buffer = audioDecoder->read();
    qint64 len = buffer.sampleCount();
    if (len > FFT_BUFFER_SIZE) {
        len = FFT_BUFFER_SIZE;
    }

    const qint16 *data = buffer.constData<qint16>();
    for (int i = 0; i < len; ++i) {
        inputBuffer[i] = static_cast<sound_sample>(data[i]);
    }

    processAudioData();
}

void AudioProcessor::handleFinished()
{
    qDebug() << "Decoding finished.";
}

void AudioProcessor::processAudioData()
{
    fft.perform(inputBuffer, outputBuffer, fftState);

    // Stars
    qDebug() << "FFT Output as star bars:";
    for (int i = 0; i < 16 && i < outputBuffer.size(); ++i) {
        // 0-10
        int numStars = static_cast<int>(
            outputBuffer[i] / *std::max_element(outputBuffer.begin(), outputBuffer.end()) * 10);
        QString stars(numStars, '*');
        qDebug() << "Bin" << i << ":" << stars;
    }

    update();
}

void AudioProcessor::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill the background with black color
    painter.fillRect(rect(), Qt::black);
    if (!paintingEnabled)
        return; // Do nothing if painting is disabled - THE STOP BUTTON IS PUSHED

    int width = this->width();
    int height = this->height();
    int barWidth = width / 16;

    // Find maximum value and normalize
    float maxValue = *std::max_element(outputBuffer.begin(), outputBuffer.end());

    if (maxValue == 0) {
        maxValue = 1; // Avoid division by zero
    }

    // Set brush color to red for drawing the bars
    painter.setBrush(Qt::green);

    for (int i = 0; i < 16 && i < outputBuffer.size(); ++i) {
        float normalizedValue = outputBuffer[i] / maxValue;
        int barHeight = static_cast<int>(normalizedValue * height);

        qDebug() << "Bar" << i << ":" << outputBuffer[i] << "Normalized:" << normalizedValue
                 << "Height:" << barHeight;

        // Draw the bar with the calculated height
        painter.drawRect(i * barWidth, height - barHeight, barWidth - 1, barHeight);
    }
}
