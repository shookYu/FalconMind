#!/bin/bash
# 端到端演示脚本：启动 Viewer 后端、前端，并提示启动 Cluster Center Mock 和 NodeAgent

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VIEWER_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BACKEND_DIR="$VIEWER_DIR/backend"
FRONTEND_DIR="$VIEWER_DIR/frontend"

echo "=========================================="
echo "FalconMind Viewer 端到端演示启动脚本"
echo "=========================================="
echo ""

# 检查 Python 环境
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到 python3"
    exit 1
fi

# 启动 Viewer 后端
echo "[1/3] 启动 Viewer 后端..."
cd "$BACKEND_DIR"

if [ ! -d ".venv" ]; then
    echo "  创建 Python 虚拟环境..."
    python3 -m venv .venv
fi

source .venv/bin/activate
pip install -q -r requirements.txt

echo "  启动后端服务 (http://127.0.0.1:9000)..."
uvicorn main:app --host 0.0.0.0 --port 9000 &
BACKEND_PID=$!
sleep 2

# 检查后端是否启动成功
if ! curl -s http://127.0.0.1:9000/health > /dev/null; then
    echo "  错误: Viewer 后端启动失败"
    kill $BACKEND_PID 2>/dev/null || true
    exit 1
fi
echo "  ✅ Viewer 后端已启动 (PID: $BACKEND_PID)"

# 启动 Viewer 前端
echo ""
echo "[2/3] 启动 Viewer 前端..."
cd "$FRONTEND_DIR"

echo "  启动前端 HTTP 服务器 (http://127.0.0.1:8000)..."
python3 -m http.server 8000 &
FRONTEND_PID=$!
sleep 1
echo "  ✅ Viewer 前端已启动 (PID: $FRONTEND_PID)"

# 显示启动信息
echo ""
echo "=========================================="
echo "✅ Viewer 已启动！"
echo "=========================================="
echo ""
echo "前端地址: http://127.0.0.1:8000/index.html"
echo "后端地址: http://127.0.0.1:9000"
echo ""
echo "下一步："
echo "1. 在浏览器中打开: http://127.0.0.1:8000/index.html"
echo ""
echo "2. 启动 Cluster Center Mock（新终端）:"
echo "   cd NodeAgent/build"
echo "   ./cluster_center_mock 8888 http://127.0.0.1:9000/ingress/telemetry true"
echo ""
echo "3. 启动 NodeAgent（新终端）:"
echo "   cd NodeAgent/build"
echo "   ./test_telemetry_flow 127.0.0.1 8888"
echo ""
echo "或者使用 SDK Telemetry Demo:"
echo "   cd FalconMindSDK/build"
echo "   ./falconmind_telemetry_demo"
echo ""
echo "按 Ctrl+C 停止 Viewer 服务"
echo "=========================================="

# 等待用户中断
trap "echo ''; echo '正在停止服务...'; kill $BACKEND_PID $FRONTEND_PID 2>/dev/null || true; exit 0" INT TERM

wait
