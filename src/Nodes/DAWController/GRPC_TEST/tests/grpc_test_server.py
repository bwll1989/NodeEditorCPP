# GRPC_TEST 联调测试服务端：实现 control.v1.ControlService
# 用法: python grpc_test_server.py [port]   (默认 50051)
import math
import sys
import time
from concurrent import futures

import grpc

import control_pb2
import control_pb2_grpc

state = {
    "rps": 0.0,
    "pos": control_pb2.Vec3(x=0, y=0, z=0),
    "euler": control_pb2.Vec3(x=0, y=0, z=0),
}


class ControlServicer(control_pb2_grpc.ControlServiceServicer):

    def SetCylinderSpeed(self, request, context):
        print(f"[server] SetCylinderSpeed: rps={request.revolutions_per_second}", flush=True)
        state["rps"] = request.revolutions_per_second
        return control_pb2.SetCylinderSpeedResponse()

    def SetCameraPose(self, request, context):
        p, e = request.position, request.euler_degrees
        print(f"[server] SetCameraPose: pos=({p.x},{p.y},{p.z}) "
              f"euler=({e.x},{e.y},{e.z})", flush=True)
        state["pos"].CopyFrom(request.position)
        state["euler"].CopyFrom(request.euler_degrees)
        return control_pb2.SetCameraPoseResponse()

    def GetState(self, request, context):
        print("[server] GetState", flush=True)
        return control_pb2.GetStateResponse(
            state=control_pb2.ControlState(
                revolutions_per_second=state["rps"],
                camera_position=state["pos"],
                camera_euler_degrees=state["euler"],
            ))


def serve(port):
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=8))
    control_pb2_grpc.add_ControlServiceServicer_to_server(ControlServicer(), server)
    bound = server.add_insecure_port(f"127.0.0.1:{port}")
    if bound == 0:
        print("[server] FATAL: port busy", flush=True)
        sys.exit(2)
    server.start()
    print(f"[server] ControlService listening on 127.0.0.1:{port}", flush=True)
    server.wait_for_termination()


if __name__ == "__main__":
    serve(int(sys.argv[1]) if len(sys.argv) > 1 else 50051)
