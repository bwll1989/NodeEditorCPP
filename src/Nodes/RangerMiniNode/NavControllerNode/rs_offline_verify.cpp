/**
 * @file rs_offline_verify.cpp
 * @brief 无真机验证：RS 规划自检 + 自行车模型闭环（同 Nav 的 Pure Pursuit）
 *
 * 编译（任选其一，在本目录执行）：
 *   g++ -std=c++17 -O2 -o rs_offline_verify rs_offline_verify.cpp
 *   cl /EHsc /std:c++17 /O2 rs_offline_verify.cpp
 *
 * 运行：
 *   ./rs_offline_verify
 *   ./rs_offline_verify --csv out.csv   # 导出第一组闭环轨迹，可用 Excel/Python 画图
 *
 * 通过标准：规划 ok + 闭环在时限内进入 (arriveDist, yawTol)。
 */

#include "RsPath.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

using Nodes::Rs::Pose2d;
using Nodes::Rs::PathResult;
using Nodes::Rs::SamplePoint;
using Nodes::Rs::plan;
using Nodes::Rs::wrapPi;

// ---- 与 NavController 默认面板一致的参数 ----
struct Params
{
    double wheelbase = 0.494; // 物理轴距 L
    double maxSteer = 0.50;   // rad
    double maxVx = 0.30;      // m/s
    double lookAhead = 0.45;  // m
    double slowDist = 0.80;   // m
    double arriveDist = 0.15; // m
    double yawTol = 0.15;     // rad
    double dt = 0.05;         // s，仿真步长（约 20 Hz）
    double timeLimit = 60.0;  // s
};

static double lEff(const Params &p) { return 0.5 * p.wheelbase; }

static double rMin(const Params &p)
{
    return lEff(p) / std::tan(std::max(0.05, p.maxSteer));
}

/** Pure Pursuit（与 NavControllerDataModel::purePursuit 同公式） */
static int purePursuit(const PathResult &path, const Pose2d &pose, const Params &p,
                       double &vx, double &steer, double &crossTrack)
{
    vx = 0.0;
    steer = 0.0;
    crossTrack = 0.0;
    if (path.samples.empty()) {
        return 1;
    }

    size_t nearest = 0;
    double bestD = 1e300;
    for (size_t i = 0; i < path.samples.size(); ++i) {
        const double d = std::hypot(path.samples[i].x - pose.x, path.samples[i].y - pose.y);
        if (d < bestD) {
            bestD = d;
            nearest = i;
        }
    }
    crossTrack = bestD;
    const int gear = path.samples[nearest].gear >= 0 ? 1 : -1;

    const double Ld = std::max(0.15, p.lookAhead);
    size_t target = nearest;
    double acc = 0.0;
    for (size_t i = nearest; i + 1 < path.samples.size(); ++i) {
        acc += std::hypot(path.samples[i + 1].x - path.samples[i].x,
                          path.samples[i + 1].y - path.samples[i].y);
        target = i + 1;
        if (acc >= Ld) {
            break;
        }
    }

    const auto &tp = path.samples[target];
    const double dx = tp.x - pose.x;
    const double dy = tp.y - pose.y;
    const double c = std::cos(pose.yaw);
    const double s = std::sin(pose.yaw);
    const double localY = -s * dx + c * dy;
    const double localX = c * dx + s * dy;
    const double alpha = (gear >= 0) ? std::atan2(localY, localX)
                                     : std::atan2(-localY, -localX);

    steer = std::atan2(2.0 * lEff(p) * std::sin(alpha), Ld);
    if (gear < 0) {
        steer = -steer;
    }
    steer = std::clamp(steer, -p.maxSteer, p.maxSteer);

    const double distGoal = std::hypot(path.samples.back().x - pose.x,
                                       path.samples.back().y - pose.y);
    const double speedScale = std::clamp(distGoal / std::max(0.05, p.slowDist), 0.15, 1.0);
    constexpr double kPi = 3.14159265358979323846;
    const double turnScale = std::clamp(1.0 - 0.5 * (std::abs(alpha) / kPi), 0.25, 1.0);
    vx = static_cast<double>(gear) * p.maxVx * speedScale * turnScale;
    (void)localX;
    return gear;
}

/**
 * 前后对向阿克曼自行车：θ̇ = v · tan(δ) / L_eff
 * 用一阶欧拉积分假车位姿（无实物时的「底盘」）
 */
static void integrateBicycle(Pose2d &pose, double vx, double steer, double L_eff, double dt)
{
    const double thDot = vx * std::tan(steer) / std::max(1e-6, L_eff);
    pose.x += vx * std::cos(pose.yaw) * dt;
    pose.y += vx * std::sin(pose.yaw) * dt;
    pose.yaw = wrapPi(pose.yaw + thDot * dt);
}

struct CaseResult
{
    std::string name;
    bool planOk = false;
    bool arrived = false;
    std::string types;
    double pathLen = 0.0;
    double time = 0.0;
    double finalDist = 0.0;
    double finalYawErr = 0.0;
    double maxCross = 0.0;
    int samples = 0;
};

struct Case
{
    const char *name;
    Pose2d start;
    Pose2d goal;
    bool expectReverse = false; // 期望路径含倒车段
};

static CaseResult runCase(const Case &c, const Params &p, std::vector<Pose2d> *traj)
{
    CaseResult r;
    r.name = c.name;

    PathResult path = plan(c.start, c.goal, rMin(p), 0.08);
    r.planOk = path.ok;
    r.types = path.typeString;
    r.pathLen = path.length;
    r.samples = static_cast<int>(path.samples.size());

    if (!path.ok) {
        return r;
    }

    Pose2d pose = c.start;
    double t = 0.0;
    double maxCross = 0.0;
    double sinceReplan = 0.0;
    const double replanPeriod = 0.5;

    while (t < p.timeLimit) {
        const double dist = std::hypot(c.goal.x - pose.x, c.goal.y - pose.y);
        const double yawErr = wrapPi(c.goal.yaw - pose.yaw);
        if (dist <= p.arriveDist && std::abs(yawErr) <= p.yawTol) {
            r.arrived = true;
            r.finalDist = dist;
            r.finalYawErr = yawErr;
            break;
        }

        if (sinceReplan >= replanPeriod) {
            path = plan(pose, c.goal, rMin(p), 0.08);
            sinceReplan = 0.0;
            if (path.ok) {
                r.types = path.typeString;
                r.pathLen = path.length;
                r.samples = static_cast<int>(path.samples.size());
            }
        }

        double vx = 0.0;
        double steer = 0.0;
        double cross = 0.0;
        if (path.ok) {
            purePursuit(path, pose, p, vx, steer, cross);
            maxCross = std::max(maxCross, cross);
            // 近终点航向纠偏（与 NavController 一致）
            if (dist < std::max(0.40, 2.5 * p.arriveDist) && std::abs(yawErr) > p.yawTol) {
                const double creep = std::copysign(std::min(0.12, p.maxVx), vx == 0.0 ? 1.0 : vx);
                const double thetaDot = 1.5 * yawErr;
                vx = creep;
                steer = std::clamp(std::atan(thetaDot * lEff(p) / creep), -p.maxSteer, p.maxSteer);
            }
            if (cross > std::max(0.6, 3.0 * p.arriveDist)) {
                path = plan(pose, c.goal, rMin(p), 0.08);
                sinceReplan = 0.0;
            }
        }
        integrateBicycle(pose, vx, steer, lEff(p), p.dt);
        if (traj) {
            traj->push_back(pose);
        }
        t += p.dt;
        sinceReplan += p.dt;
    }

    r.time = t;
    r.maxCross = maxCross;
    if (!r.arrived) {
        r.finalDist = std::hypot(c.goal.x - pose.x, c.goal.y - pose.y);
        r.finalYawErr = wrapPi(c.goal.yaw - pose.yaw);
    }
    return r;
}

static bool hasReverse(const std::string &types)
{
    return types.find('-') != std::string::npos;
}

int main(int argc, char **argv)
{
    std::string csvPath;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csvPath = argv[++i];
        }
    }

    Params p;
    const double r = rMin(p);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "=== RS + Pure Pursuit 离线验证（无真机）===\n"
              << "L=" << p.wheelbase << "  L_eff=" << lEff(p)
              << "  R_min=" << r << "  MaxVx=" << p.maxVx << "\n\n";

    // 典型几何：前方直行、侧向、后方（应倒车）、平行换向
    const Case cases[] = {
        {"A_forward_1m", {0, 0, 0}, {1.0, 0.0, 0.0}, false},
        {"B_side_1m", {0, 0, 0}, {1.0, 0.8, 0.0}, false},
        {"C_behind_1m", {0, 0, 0}, {-1.0, 0.0, 0.0}, true},
        {"D_parallel_yaw", {0, 0, 0}, {0.0, 1.0, 1.5708}, true},
        {"E_turn_inplace_ish", {0, 0, 0}, {0.2, 0.2, 1.5708}, false},
    };

    int fail = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        std::vector<Pose2d> traj;
        const bool dump = (i == 0 && !csvPath.empty());
        CaseResult r = runCase(cases[i], p, dump ? &traj : nullptr);

        const bool revOk = !cases[i].expectReverse || hasReverse(r.types);
        const bool pass = r.planOk && r.arrived && revOk;
        if (!pass) {
            ++fail;
        }

        std::cout << (pass ? "[PASS] " : "[FAIL] ") << r.name
                  << "  plan=" << (r.planOk ? "ok" : "NO")
                  << "  types=" << (r.types.empty() ? "-" : r.types)
                  << "  len=" << r.pathLen << "m"
                  << "  n=" << r.samples
                  << "  arrived=" << (r.arrived ? "yes" : "NO")
                  << "  t=" << r.time << "s"
                  << "  d=" << r.finalDist
                  << "  yawErr=" << r.finalYawErr
                  << "  maxCT=" << r.maxCross
                  << "\n";

        if (cases[i].expectReverse && !hasReverse(r.types)) {
            std::cout << "         !! 期望含倒车段(-)，实际 types 无 '-'\n";
        }

        if (dump && !traj.empty()) {
            std::ofstream ofs(csvPath);
            ofs << "t,x,y,yaw\n";
            for (size_t k = 0; k < traj.size(); ++k) {
                ofs << (k * p.dt) << "," << traj[k].x << "," << traj[k].y << "," << traj[k].yaw << "\n";
            }
            std::cout << "  CSV 已写: " << csvPath << " (" << traj.size() << " 点)\n";
        }
    }

    std::cout << "\n结果: " << (fail == 0 ? "全部通过" : std::to_string(fail) + " 组失败")
              << " / " << (sizeof(cases) / sizeof(cases[0])) << "\n";
    std::cout << "说明: 这是理想自行车模型，通过≠实车一定稳，但 FAIL 说明算法/参数有问题。\n";
    return fail == 0 ? 0 : 1;
}
