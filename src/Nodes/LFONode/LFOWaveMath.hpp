#pragma once

#include <QtMath>
#include <algorithm>

namespace Nodes
{
    /** TouchDesigner LFO CHOP 风格单位波形，输出再经 offset + amplitude * unit */
    enum class LFOWaveType : int {
        Sine = 0,
        Square = 1,
        Triangle = 2,
        Gaussian = 3,
        Ramp = 4,
        Pulse = 5
    };

    inline bool lfoWaveIsBipolar(LFOWaveType type)
    {
        switch (type) {
        case LFOWaveType::Sine:
        case LFOWaveType::Square:
        case LFOWaveType::Triangle:
            return true;
        case LFOWaveType::Gaussian:
        case LFOWaveType::Ramp:
        case LFOWaveType::Pulse:
            return false;
        }
        return true;
    }

    inline double lfoNormalizedPhase(double period, double time, double phase)
    {
        if (qFuzzyIsNull(period)) {
            return 0.0;
        }
        double x = std::fmod(time / period + phase, 1.0);
        if (x < 0.0) {
            x += 1.0;
        }
        return x;
    }

    /**
     * 单位波采样。x 为归一化相位 [0,1)；bias ∈ [-1,1]。
     * - Sine / Triangle / Square：约 [-1, 1]
     * - Gaussian / Ramp / Pulse：约 [0, 1]
     * Bias：Triangle 移动峰值；Square 改变占空比；Gaussian 改变宽度。
     */
    inline double lfoSampleUnit(LFOWaveType type, double x, double bias, double pulseWidth = 0.02)
    {
        const double b = qBound(-1.0, bias, 1.0);

        switch (type) {
        case LFOWaveType::Sine:
            return qSin(2.0 * M_PI * x);

        case LFOWaveType::Gaussian: {
            // 钟形居中；bias=-1 最宽，+1 最窄（仍保持可见，避免尖到看不见）
            // sigma ∈ 约 [0.08, 0.32]
            const double sigma = 0.20 * qPow(2.0, -b);
            const double d = x - 0.5;
            return qExp(-(d * d) / (2.0 * sigma * sigma));
        }

        case LFOWaveType::Triangle: {
            // bias=0 峰值在 0.5；负偏左、正偏右
            const double peak = qBound(0.02, 0.5 + 0.5 * b, 0.98);
            if (x < peak) {
                return -1.0 + 2.0 * (x / peak);
            }
            return 1.0 - 2.0 * ((x - peak) / (1.0 - peak));
        }

        case LFOWaveType::Ramp:
            return x;

        case LFOWaveType::Square: {
            // bias=0 → 50% 占空比
            const double duty = qBound(0.02, 0.5 + 0.5 * b, 0.98);
            return x < duty ? 1.0 : -1.0;
        }

        case LFOWaveType::Pulse: {
            const double w = qBound(1e-4, pulseWidth, 0.5);
            return x < w ? 1.0 : 0.0;
        }
        }

        return 0.0;
    }

    inline double lfoSample(LFOWaveType type,
                            double period,
                            double time,
                            double phase,
                            double amplitude,
                            double offset,
                            double bias,
                            double pulseWidth = 0.02)
    {
        const double x = lfoNormalizedPhase(period, time, phase);
        const double unit = lfoSampleUnit(type, x, bias, pulseWidth);
        return offset + amplitude * unit;
    }
}
