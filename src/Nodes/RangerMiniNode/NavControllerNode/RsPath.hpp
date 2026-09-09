#pragma once

/**
 * @file RsPath.hpp
 * @brief Reeds–Shepp 最短路径规划（含前进/倒车圆弧与直线）
 *
 * 公式来自 Reeds & Shepp (1990)，实现结构参考常见开源移植（CSC/CCC/CCSC 等族
 * + timeflip/reflect 对称）。在给定最小转弯半径 R_min 下，对起终点位姿
 * (x,y,θ) 求最短可行路径并按弧长采样。
 *
 * 单位：米、弧度；曲率 maxc = 1/R_min。
 */

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

namespace Nodes
{
namespace Rs
{
    struct Pose2d
    {
        double x = 0.0;
        double y = 0.0;
        double yaw = 0.0;
    };

    /// 采样点：gear = +1 前进，-1 倒车
    struct SamplePoint
    {
        double x = 0.0;
        double y = 0.0;
        double yaw = 0.0;
        int gear = 1;
    };

    struct PathResult
    {
        bool ok = false;
        double length = 0.0;              // 总弧长 m（各段绝对值之和）
        std::vector<char> ctypes;         // 'L'/'R'/'S'
        std::vector<double> lengths;      // 各段有符号长度 m（负=倒车）
        std::vector<SamplePoint> samples; // 世界系采样
        std::string typeString;           // 如 "L+S+R-"
    };

    inline double wrapPi(double a)
    {
        constexpr double kPi = 3.14159265358979323846;
        a = std::fmod(a + kPi, 2.0 * kPi);
        if (a < 0.0) {
            a += 2.0 * kPi;
        }
        return a - kPi;
    }

    inline double mod2pi(double x)
    {
        constexpr double kPi = 3.14159265358979323846;
        double v = std::fmod(x, std::copysign(2.0 * kPi, x));
        if (v < -kPi) {
            v += 2.0 * kPi;
        } else if (v > kPi) {
            v -= 2.0 * kPi;
        }
        return v;
    }

    inline void polar(double x, double y, double &r, double &theta)
    {
        r = std::hypot(x, y);
        theta = std::atan2(y, x);
    }

    inline double clampAcos(double v)
    {
        return std::acos(std::clamp(v, -1.0, 1.0));
    }

    // -------------------- 归一化坐标系下的路径族（曲率=1）--------------------

    inline bool leftStraightLeft(double x, double y, double phi,
                                 std::vector<double> &len, std::vector<char> &typ)
    {
        double u = 0.0;
        double t = 0.0;
        polar(x - std::sin(phi), y - 1.0 + std::cos(phi), u, t);
        constexpr double kPi = 3.14159265358979323846;
        if (t >= 0.0 && t <= kPi) {
            const double v = mod2pi(phi - t);
            if (v >= 0.0 && v <= kPi) {
                len = {t, u, v};
                typ = {'L', 'S', 'L'};
                return true;
            }
        }
        return false;
    }

    inline bool leftStraightRight(double x, double y, double phi,
                                  std::vector<double> &len, std::vector<char> &typ)
    {
        double u1 = 0.0;
        double t1 = 0.0;
        polar(x + std::sin(phi), y - 1.0 - std::cos(phi), u1, t1);
        u1 = u1 * u1;
        if (u1 >= 4.0) {
            const double u = std::sqrt(u1 - 4.0);
            const double theta = std::atan2(2.0, u);
            const double t = mod2pi(t1 + theta);
            const double v = mod2pi(t - phi);
            if (t >= 0.0 && v >= 0.0) {
                len = {t, u, v};
                typ = {'L', 'S', 'R'};
                return true;
            }
        }
        return false;
    }

    inline bool leftXRightXLeft(double x, double y, double phi,
                                std::vector<double> &len, std::vector<char> &typ)
    {
        const double zeta = x - std::sin(phi);
        const double eeta = y - 1.0 + std::cos(phi);
        double u1 = 0.0;
        double theta = 0.0;
        polar(zeta, eeta, u1, theta);
        constexpr double kPi = 3.14159265358979323846;
        if (u1 <= 4.0) {
            const double A = clampAcos(0.25 * u1);
            const double t = mod2pi(A + theta + kPi / 2.0);
            const double u = mod2pi(kPi - 2.0 * A);
            const double v = mod2pi(phi - t - u);
            len = {t, -u, v};
            typ = {'L', 'R', 'L'};
            return true;
        }
        return false;
    }

    inline bool leftXRightLeft(double x, double y, double phi,
                               std::vector<double> &len, std::vector<char> &typ)
    {
        const double zeta = x - std::sin(phi);
        const double eeta = y - 1.0 + std::cos(phi);
        double u1 = 0.0;
        double theta = 0.0;
        polar(zeta, eeta, u1, theta);
        constexpr double kPi = 3.14159265358979323846;
        if (u1 <= 4.0) {
            const double A = clampAcos(0.25 * u1);
            const double t = mod2pi(A + theta + kPi / 2.0);
            const double u = mod2pi(kPi - 2.0 * A);
            const double v = mod2pi(-phi + t + u);
            len = {t, -u, -v};
            typ = {'L', 'R', 'L'};
            return true;
        }
        return false;
    }

    inline bool leftRightXLeft(double x, double y, double phi,
                               std::vector<double> &len, std::vector<char> &typ)
    {
        const double zeta = x - std::sin(phi);
        const double eeta = y - 1.0 + std::cos(phi);
        double u1 = 0.0;
        double theta = 0.0;
        polar(zeta, eeta, u1, theta);
        constexpr double kPi = 3.14159265358979323846;
        if (u1 <= 4.0 && u1 > 1e-9) {
            const double u = clampAcos(1.0 - u1 * u1 * 0.125);
            const double A = std::asin(std::clamp(2.0 * std::sin(u) / u1, -1.0, 1.0));
            const double t = mod2pi(-A + theta + kPi / 2.0);
            const double v = mod2pi(t - u - phi);
            len = {t, u, -v};
            typ = {'L', 'R', 'L'};
            return true;
        }
        return false;
    }

    inline bool leftXRight90StraightLeft(double x, double y, double phi,
                                         std::vector<double> &len, std::vector<char> &typ)
    {
        const double zeta = x - std::sin(phi);
        const double eeta = y - 1.0 + std::cos(phi);
        double u1 = 0.0;
        double theta = 0.0;
        polar(zeta, eeta, u1, theta);
        constexpr double kPi = 3.14159265358979323846;
        if (u1 >= 2.0) {
            const double u = std::sqrt(u1 * u1 - 4.0) - 2.0;
            const double A = std::atan2(2.0, std::sqrt(u1 * u1 - 4.0));
            const double t = mod2pi(theta + A + kPi / 2.0);
            const double v = mod2pi(t - phi + kPi / 2.0);
            if (t >= 0.0 && v >= 0.0) {
                len = {t, -kPi / 2.0, -u, -v};
                typ = {'L', 'R', 'S', 'L'};
                return true;
            }
        }
        return false;
    }

    inline bool leftXRight90StraightRight(double x, double y, double phi,
                                          std::vector<double> &len, std::vector<char> &typ)
    {
        const double zeta = x + std::sin(phi);
        const double eeta = y - 1.0 - std::cos(phi);
        double u1 = 0.0;
        double theta = 0.0;
        polar(zeta, eeta, u1, theta);
        constexpr double kPi = 3.14159265358979323846;
        if (u1 >= 2.0) {
            const double t = mod2pi(theta + kPi / 2.0);
            const double u = u1 - 2.0;
            const double v = mod2pi(phi - t - kPi / 2.0);
            if (t >= 0.0 && v >= 0.0) {
                len = {t, -kPi / 2.0, -u, -v};
                typ = {'L', 'R', 'S', 'R'};
                return true;
            }
        }
        return false;
    }

    inline bool leftStraightRight90XLeft(double x, double y, double phi,
                                         std::vector<double> &len, std::vector<char> &typ)
    {
        const double zeta = x - std::sin(phi);
        const double eeta = y - 1.0 + std::cos(phi);
        double u1 = 0.0;
        double theta = 0.0;
        polar(zeta, eeta, u1, theta);
        constexpr double kPi = 3.14159265358979323846;
        if (u1 >= 2.0) {
            const double u = std::sqrt(u1 * u1 - 4.0) - 2.0;
            const double A = std::atan2(std::sqrt(u1 * u1 - 4.0), 2.0);
            const double t = mod2pi(theta - A + kPi / 2.0);
            const double v = mod2pi(t - phi - kPi / 2.0);
            if (t >= 0.0 && v >= 0.0) {
                len = {t, u, kPi / 2.0, -v};
                typ = {'L', 'S', 'R', 'L'};
                return true;
            }
        }
        return false;
    }

    using PathFunc = bool (*)(double, double, double, std::vector<double> &, std::vector<char> &);

    inline std::vector<double> timeflip(const std::vector<double> &d)
    {
        std::vector<double> o;
        o.reserve(d.size());
        for (double x : d) {
            o.push_back(-x);
        }
        return o;
    }

    inline std::vector<char> reflect(const std::vector<char> &d)
    {
        std::vector<char> o;
        o.reserve(d.size());
        for (char c : d) {
            if (c == 'L') {
                o.push_back('R');
            } else if (c == 'R') {
                o.push_back('L');
            } else {
                o.push_back('S');
            }
        }
        return o;
    }

    struct Cand
    {
        std::vector<double> lengths; // 归一化有符号长度
        std::vector<char> ctypes;
        double L = 0.0;
    };

    /// 去掉数值上为零的段，避免 gear 误判与 types 虚增（如 L+S+L+ 实为 S+）
    inline void sanitizeSegments(std::vector<double> &lengths, std::vector<char> &ctypes)
    {
        std::vector<double> nl;
        std::vector<char> nt;
        nl.reserve(lengths.size());
        nt.reserve(ctypes.size());
        for (size_t i = 0; i < lengths.size(); ++i) {
            if (std::abs(lengths[i]) > 1e-6) {
                nl.push_back(lengths[i]);
                nt.push_back(ctypes[i]);
            }
        }
        lengths.swap(nl);
        ctypes.swap(nt);
    }

    inline void tryAdd(std::vector<Cand> &out, std::vector<double> lengths,
                       std::vector<char> ctypes, double stepNorm)
    {
        sanitizeSegments(lengths, ctypes);
        double L = 0.0;
        for (double d : lengths) {
            L += std::abs(d);
        }
        // 允许短路径（近点微调）；仅丢弃数值上为零的候选
        if (L < 1e-9 || lengths.empty()) {
            return;
        }
        for (const Cand &c : out) {
            if (c.ctypes == ctypes && std::abs(c.L - L) <= stepNorm) {
                return;
            }
        }
        Cand c;
        c.lengths = std::move(lengths);
        c.ctypes = std::move(ctypes);
        c.L = L;
        out.push_back(std::move(c));
    }

    inline void interpolate(double dist, double length, char mode, double maxc,
                            double ox, double oy, double oyaw,
                            double &x, double &y, double &yaw, int &gear)
    {
        // 零长度按前进处理，避免起点采样 gear=-1 导致倒车跑飞
        gear = length >= 0.0 ? 1 : -1;
        if (mode == 'S') {
            x = ox + dist / maxc * std::cos(oyaw);
            y = oy + dist / maxc * std::sin(oyaw);
            yaw = oyaw;
            return;
        }
        const double ldx = std::sin(dist) / maxc;
        double ldy = 0.0;
        if (mode == 'L') {
            ldy = (1.0 - std::cos(dist)) / maxc;
            yaw = oyaw + dist;
        } else { // 'R'
            ldy = (1.0 - std::cos(dist)) / -maxc;
            yaw = oyaw - dist;
        }
        const double gdx = std::cos(-oyaw) * ldx + std::sin(-oyaw) * ldy;
        const double gdy = -std::sin(-oyaw) * ldx + std::cos(-oyaw) * ldy;
        x = ox + gdx;
        y = oy + gdy;
    }

    /**
     * @brief 规划并采样 Reeds–Shepp 路径
     * @param start / goal  世界系位姿
     * @param turningRadius 最小转弯半径 R_min = L_eff / tan(δ_max) > 0
     * @param stepSize      采样弧长步长 m
     */
    inline PathResult plan(const Pose2d &start, const Pose2d &goal,
                           double turningRadius, double stepSize = 0.08)
    {
        PathResult result;
        if (turningRadius < 1e-3 || stepSize < 1e-4) {
            return result;
        }
        const double maxc = 1.0 / turningRadius;

        const double dx = goal.x - start.x;
        const double dy = goal.y - start.y;
        const double c0 = std::cos(start.yaw);
        const double s0 = std::sin(start.yaw);
        // 归一化到起点局部系，并按曲率缩放
        const double x = (c0 * dx + s0 * dy) * maxc;
        const double y = (-s0 * dx + c0 * dy) * maxc;
        const double dth = wrapPi(goal.yaw - start.yaw);
        const double stepNorm = stepSize * maxc;

        static const PathFunc kFuncs[] = {
            leftStraightLeft,
            leftStraightRight,
            leftXRightXLeft,
            leftXRightLeft,
            leftRightXLeft,
            leftXRight90StraightLeft,
            leftXRight90StraightRight,
            leftStraightRight90XLeft,
        };

        std::vector<Cand> cands;
        for (PathFunc fn : kFuncs) {
            std::vector<double> len;
            std::vector<char> typ;
            if (fn(x, y, dth, len, typ)) {
                tryAdd(cands, len, typ, stepNorm);
            }
            if (fn(-x, y, -dth, len, typ)) {
                tryAdd(cands, timeflip(len), typ, stepNorm);
            }
            if (fn(x, -y, -dth, len, typ)) {
                tryAdd(cands, len, reflect(typ), stepNorm);
            }
            if (fn(-x, -y, dth, len, typ)) {
                tryAdd(cands, timeflip(len), reflect(typ), stepNorm);
            }
        }

        if (cands.empty()) {
            return result;
        }

        const Cand *best = &cands.front();
        for (const Cand &c : cands) {
            if (c.L < best->L) {
                best = &c;
            }
        }

        // 局部系采样（归一化长度），再变换到世界系
        double ox = 0.0;
        double oy = 0.0;
        double oyaw = 0.0;
        std::vector<SamplePoint> local;
        for (size_t i = 0; i < best->lengths.size(); ++i) {
            const double length = best->lengths[i];
            const char mode = best->ctypes[i];
            // 跳过零长度段（sanitize 后理论上已无，双保险）
            if (std::abs(length) < 1e-9) {
                continue;
            }
            const double dDist = length >= 0.0 ? stepNorm : -stepNorm;
            for (double dist = 0.0;; dist += dDist) {
                const bool last = (dDist > 0.0) ? (dist >= length) : (dist <= length);
                const double use = last ? length : dist;
                double px = 0.0;
                double py = 0.0;
                double pyaw = 0.0;
                int gear = 1;
                interpolate(use, length, mode, maxc, ox, oy, oyaw, px, py, pyaw, gear);
                SamplePoint sp;
                sp.x = px;
                sp.y = py;
                sp.yaw = pyaw;
                sp.gear = gear;
                local.push_back(sp);
                if (last) {
                    ox = px;
                    oy = py;
                    oyaw = pyaw;
                    break;
                }
            }
        }

        result.ok = true;
        result.length = best->L / maxc;
        result.ctypes = best->ctypes;
        result.lengths.clear();
        for (double L : best->lengths) {
            result.lengths.push_back(L / maxc);
        }
        // 如 "L+S+R-"：段类型 + 前进(+) / 倒车(-)
        for (size_t i = 0; i < result.ctypes.size(); ++i) {
            result.typeString.push_back(result.ctypes[i]);
            result.typeString.push_back(result.lengths[i] >= 0.0 ? '+' : '-');
        }

        result.samples.reserve(local.size());
        for (const SamplePoint &p : local) {
            SamplePoint w;
            // 局部系（以 start 为原点、朝向为 x）→ 世界系
            w.x = c0 * p.x - s0 * p.y + start.x;
            w.y = s0 * p.x + c0 * p.y + start.y;
            w.yaw = wrapPi(p.yaw + start.yaw);
            w.gear = p.gear;
            result.samples.push_back(w);
        }

        // 确保终点贴合目标（数值误差）；档位继承末段
        if (!result.samples.empty()) {
            const int lastGear = result.samples.back().gear;
            result.samples.back().x = goal.x;
            result.samples.back().y = goal.y;
            result.samples.back().yaw = goal.yaw;
            result.samples.back().gear = lastGear;
        }
        return result;
    }

} // namespace Rs
} // namespace Nodes
