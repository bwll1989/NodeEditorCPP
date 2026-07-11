#pragma once

#include <QObject>
#include <QMutex>
#include <vector>
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "Gist.h"

namespace Nodes
{
    struct AudioAnalysisParams
    {
        float lowMinHz = 20.0f;
        float lowMaxHz = 250.0f;
        float midMinHz = 250.0f;
        float midMaxHz = 4000.0f;
        float highMinHz = 4000.0f;
        float highMaxHz = 20000.0f;
        int frameSize = 2048;
        int beatIntervalMs = 120;
        float bandAttackMs = 20.0f;
        float bandReleaseMs = 120.0f;
        float beatFluxMultiplier = 1.8f;
        float beatFluxSmoothing = 0.92f;
    };

    class AudioAnalysisWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioAnalysisWorker(QObject *parent = nullptr);
        ~AudioAnalysisWorker();

    public slots:
        void startProcessing();
        void stopProcessing();
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);
        void setAnalysisParams(float lowMinHz, float lowMaxHz,
                               float midMinHz, float midMaxHz,
                               float highMinHz, float highMaxHz,
                               int frameSize, int beatIntervalMs,
                               float bandAttackMs, float bandReleaseMs);

    signals:
        void processingStatusChanged(bool isProcessing);
        void analysisOutputsChanged(double low, double mid, double high, double level, bool beat);

    private slots:
        void onFrameTick(qint64 frameCount);

    private:
        void initializeGist(int frameSize, int sampleRate);
        void performAnalysisOperation(const AudioFrame &inputFrame);
        std::vector<float> extractMonoSamples(const AudioFrame &frame) const;
        void appendSamples(const std::vector<float> &samples);
        bool consumeAnalysisFrame(std::vector<float> &frameOut, int frameSize);
        float computeBandEnergy(const std::vector<float> &magnitudeSpectrum,
                                float lowHz, float highHz, int sampleRate) const;
        float smoothBand(float target, float current, float dtSeconds,
                         float attackMs, float releaseMs) const;
        bool detectBeat(float spectralFlux, qint64 timestampMs, const AudioAnalysisParams &params);

        std::shared_ptr<AudioTimestampRingQueue> _inputBuffers;
        QMutex _mutex;
        bool _isProcessing = false;
        qint64 _lastProcessedTimestamp = 0;

        std::unique_ptr<Gist<float>> _gistAnalyzer;
        int _sampleRate = 48000;
        AudioAnalysisParams _params;

        std::vector<float> _sampleAccumulator;
        float _lowSmoothed = 0.0f;
        float _midSmoothed = 0.0f;
        float _highSmoothed = 0.0f;

        float _fluxAverage = 0.0f;
        qint64 _lastBeatTimestamp = 0;
    };
}
