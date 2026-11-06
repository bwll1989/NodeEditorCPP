// GRPC_TEST 通道层独立联调程序：
// 对 Python grpcio 实现的 ControlService 依次发起
// SetCylinderSpeed → SetCameraPose → SetCylinderSpeed(-0.25) → GetState，校验状态回读。
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <cmath>
#include <cstdio>
#include <functional>

#include "../GrpcChannel.h"
#include "../control.pb.h"

using namespace Nodes;

static int g_failures = 0;

static QByteArray pbBytes(const google::protobuf::MessageLite &m)
{
    const std::string s = m.SerializeAsString();
    return QByteArray(s.data(), int(s.size()));
}

static void check(bool ok, const QString &what)
{
    fprintf(stderr, "[%s] %s\n", ok ? "PASS" : "FAIL", what.toLocal8Bit().constData());
    if (!ok) {
        ++g_failures;
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const int port = argc > 1 ? QString::fromLocal8Bit(argv[1]).toInt() : 50051;

    control::v1::SetCylinderSpeedRequest speed1;
    speed1.set_revolutions_per_second(1.5f);

    control::v1::SetCameraPoseRequest pose;
    pose.mutable_position()->set_x(1.0f);
    pose.mutable_position()->set_y(2.0f);
    pose.mutable_position()->set_z(3.0f);
    pose.mutable_euler_degrees()->set_x(10.0f);
    pose.mutable_euler_degrees()->set_y(20.0f);
    pose.mutable_euler_degrees()->set_z(30.0f);

    control::v1::SetCylinderSpeedRequest speed2;
    speed2.set_revolutions_per_second(-0.25f);

    GrpcChannel channel;
    QObject::connect(&channel, &GrpcChannel::connectedChanged, [port](bool connected) {
        fprintf(stderr, "[channel] connected=%d -> 127.0.0.1:%d\n", int(connected), port);
    });

    std::function<void()> stepGetState;
    std::function<void()> stepSpeed2;
    std::function<void()> stepSetPose;
    std::function<void()> stepSpeed1;

    stepGetState = [&]() {
        channel.call(QStringLiteral("/control.v1.ControlService/GetState"), QByteArray(),
                     [](int st, const QString &msg, const QByteArray &resp) {
            check(st == 0, QStringLiteral("GetState status=%1 %2").arg(st).arg(msg));
            control::v1::GetStateResponse r;
            if (!r.ParseFromString(resp.toStdString())) {
                check(false, QStringLiteral("GetState response parse"));
                QCoreApplication::exit(1);
                return;
            }
            const auto &s = r.state();
            fprintf(stderr, "  state: rps=%g pos=(%g,%g,%g) euler=(%g,%g,%g)\n",
                        s.revolutions_per_second(),
                        s.camera_position().x(), s.camera_position().y(), s.camera_position().z(),
                        s.camera_euler_degrees().x(), s.camera_euler_degrees().y(),
                        s.camera_euler_degrees().z());
            check(std::fabs(s.revolutions_per_second() - (-0.25f)) < 1e-4f,
                  QStringLiteral("GetState.rps == -0.25"));
            check(std::fabs(s.camera_position().x() - 1.f) < 1e-4f
                      && std::fabs(s.camera_position().y() - 2.f) < 1e-4f
                      && std::fabs(s.camera_position().z() - 3.f) < 1e-4f,
                  QStringLiteral("GetState.position == (1,2,3)"));
            check(std::fabs(s.camera_euler_degrees().x() - 10.f) < 1e-4f
                      && std::fabs(s.camera_euler_degrees().y() - 20.f) < 1e-4f
                      && std::fabs(s.camera_euler_degrees().z() - 30.f) < 1e-4f,
                  QStringLiteral("GetState.euler == (10,20,30)"));
            QCoreApplication::exit(g_failures == 0 ? 0 : 1);
        }, 5000);
    };

    stepSpeed2 = [&]() {
        channel.call(QStringLiteral("/control.v1.ControlService/SetCylinderSpeed"),
                     pbBytes(speed2),
                     [&](int st, const QString &msg, const QByteArray &) {
            check(st == 0, QStringLiteral("SetCylinderSpeed(-0.25) status=%1 %2").arg(st).arg(msg));
            stepGetState();
        }, 5000);
    };

    stepSetPose = [&]() {
        channel.call(QStringLiteral("/control.v1.ControlService/SetCameraPose"),
                     pbBytes(pose),
                     [&](int st, const QString &msg, const QByteArray &) {
            check(st == 0, QStringLiteral("SetCameraPose status=%1 %2").arg(st).arg(msg));
            stepSpeed2();
        }, 5000);
    };

    stepSpeed1 = [&]() {
        channel.call(QStringLiteral("/control.v1.ControlService/SetCylinderSpeed"),
                     pbBytes(speed1),
                     [&](int st, const QString &msg, const QByteArray &) {
            check(st == 0, QStringLiteral("SetCylinderSpeed(1.5) status=%1 %2").arg(st).arg(msg));
            stepSetPose();
        }, 5000);
    };

    // 连接就绪后开始执行步骤链
    QObject::connect(&channel, &GrpcChannel::connectedChanged, [&](bool connected) {
        if (connected) {
            QTimer::singleShot(0, &channel, stepSpeed1);
        }
    });

    QTimer::singleShot(20000, [&]() {
        fprintf(stderr, "[FAIL] overall timeout\n");
        QCoreApplication::exit(2);
    });

    channel.connectTo(QStringLiteral("127.0.0.1"), quint16(port));
    return QCoreApplication::exec();
}
