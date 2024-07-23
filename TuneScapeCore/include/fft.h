#ifndef FFT_H
#define FFT_H

// This header is mainly based on VLC FFT implementation.
#include <QObject>
#include <QVector>

#define FFT_BUFFER_SIZE_LOG 9
#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG)

typedef qint16 sound_sample; // Assuming 16-bit signed integer samples

struct FFTState
{
    QVector<float> real;
    QVector<float> imag;
    QVector<unsigned int> bitReverse;
    QVector<float> costable;
    QVector<float> sintable;
};

class FFT : public QObject
{
    Q_OBJECT
public:
    explicit FFT(QObject *parent = nullptr);
    ~FFT();

    FFTState *init();
    void perform(QVector<short int> &input, QVector<float> &output, FFTState *state);
    void close(FFTState *state);

private:
    void prepare(const QVector<sound_sample> &input,
                 QVector<float> &re,
                 QVector<float> &im,
                 const QVector<unsigned int> &bitReverse);
    void calculate(QVector<float> &re,
                   QVector<float> &im,
                   const QVector<float> &costable,
                   const QVector<float> &sintable);
    void output(const QVector<float> &re, const QVector<float> &im, QVector<float> &output);
    unsigned int reverseBits(unsigned int initial);

    QVector<float> costable;
    QVector<float> sintable;
};

#endif // FFT_H
