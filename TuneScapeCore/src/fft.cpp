#include "include/fft.h"
#include <QDebug>
#define PI 3.14159265358979323846

FFT::FFT(QObject *parent)
    : QObject(parent)
{
    // Initialize cosine and sine tables
    costable.resize(FFT_BUFFER_SIZE / 2);
    sintable.resize(FFT_BUFFER_SIZE / 2);
    for (int i = 0; i < FFT_BUFFER_SIZE / 2; ++i) {
        float j = 2 * PI * i / FFT_BUFFER_SIZE;
        costable[i] = std::cos(j);
        sintable[i] = std::sin(j);
    }
}

FFT::~FFT()
{
    // Destructor
}

FFTState *FFT::init()
{
    FFTState *state = new FFTState;
    state->real.resize(FFT_BUFFER_SIZE);
    state->imag.resize(FFT_BUFFER_SIZE);
    state->bitReverse.resize(FFT_BUFFER_SIZE);

    // Populate bit reverse table
    for (int i = 0; i < FFT_BUFFER_SIZE; ++i) {
        state->bitReverse[i] = reverseBits(i);
    }

    // Copy cosine and sine tables
    state->costable = costable;
    state->sintable = sintable;

    return state;
}

void FFT::perform(QVector<short int> &input, QVector<float> &output, FFTState *state)
{
    // Prepare data for FFT
    prepare(input, state->real, state->imag, state->bitReverse);

    // Perform the FFT calculation
    calculate(state->real, state->imag, state->costable, state->sintable);

    // Compute magnitude or power spectrum
    output.resize(FFT_BUFFER_SIZE / 2 + 1);
    for (int i = 0; i <= FFT_BUFFER_SIZE / 2; ++i) {
        output[i] = sqrt(state->real[i] * state->real[i] + state->imag[i] * state->imag[i]);
    }
}

void FFT::close(FFTState *state)
{
    delete state;
}

void FFT::prepare(const QVector<sound_sample> &input,
                  QVector<float> &re,
                  QVector<float> &im,
                  const QVector<unsigned int> &bitReverse)
{
    for (int i = 0; i < FFT_BUFFER_SIZE; ++i) {
        re[i] = static_cast<float>(input[bitReverse[i]]);
        im[i] = 0.0;
    }
}

void FFT::calculate(QVector<float> &re,
                    QVector<float> &im,
                    const QVector<float> &costable,
                    const QVector<float> &sintable)
{
    unsigned int exchanges = 1;
    unsigned int factfact = FFT_BUFFER_SIZE / 2;

    for (int i = FFT_BUFFER_SIZE_LOG; i > 0; --i) {
        for (unsigned int j = 0; j < exchanges; ++j) {
            float fact_real = costable[j * factfact];
            float fact_imag = sintable[j * factfact];

            for (unsigned int k = j; k < FFT_BUFFER_SIZE; k += exchanges << 1) {
                unsigned int k1 = k + exchanges;
                float tmp_real = fact_real * re[k1] - fact_imag * im[k1];
                float tmp_imag = fact_real * im[k1] + fact_imag * re[k1];
                re[k1] = re[k] - tmp_real;
                im[k1] = im[k] - tmp_imag;
                re[k] += tmp_real;
                im[k] += tmp_imag;
            }
        }
        exchanges <<= 1;
        factfact >>= 1;
    }
}

void FFT::output(const QVector<float> &re, const QVector<float> &im, QVector<float> &output)
{
    output.resize(FFT_BUFFER_SIZE / 2 + 1);
    for (int i = 0; i <= FFT_BUFFER_SIZE / 2; ++i) {
        output[i] = (re[i] * re[i]) + (im[i] * im[i]);
    }
    output[0] /= 4;
    output[FFT_BUFFER_SIZE / 2] /= 4;
}

unsigned int FFT::reverseBits(unsigned int initial)
{
    unsigned int reversed = 0;
    for (int loop = 0; loop < FFT_BUFFER_SIZE_LOG; ++loop) {
        reversed <<= 1;
        reversed += (initial & 1);
        initial >>= 1;
    }
    return reversed;
}
