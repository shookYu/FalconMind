#!/bin/bash
# 启动 FalconMindBuilder 服务

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILDER_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BACKEND_DIR="$BUILDER_DIR/backend"
FRONTEND_DIR="$BUILDER_DIR/frontend"

echo "=========================================="
echo "FalconMindBuilder 启动脚本"
echo "=========================================="
echo ""

# 检查 Python 环境
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到 python3"
    exit 1
fi

# 启动后端
echo "[1/2] 启动 Builder 后端..."
cd "$BACKEND_DIR"

if [ ! -d ".venv" ]; then
    echo "  创建 Python 虚拟环境..."
    python3 -m venv .venv
fi

source .venv/bin/activate
pip install -q -r requirements.txt

echo "  启动后端服务 (http://127.0.0.1:9001)..."
uvicorn main:app --host 0.0.0.0 --port 9001 &
BACKEND_PID=$!
sleep 2

# 检查后端是否启动成功
if ! curl -s http://127.0.0.1:9001/health > /dev/null; then
    echo "  错误: Builder 后端启动失败"
    kill $BACKEND_PID 2>/dev/null || true
    exit 1
fi
echo "  ✅ Builder 后端已启动 (PID: $BACKEND_PID)"

# 启动前端
echo ""
echo "[2/2] 启动 Builder 前端..."
cd "$FRONTEND_DIR"

echo "  启动前端 HTTP 服务器 (http://127.0.0.1:8001)..."
python3 -m http.server 8001 &
FRONTEND_PID=$!
sleep 1
echo "  ✅ Builder 前端已启动 (PID: $FRONTEND_PID)"

# 显示启动信息
echo ""
echo "=========================================="
echo "✅ FalconMindBuilder 已启动！"
echo "=========================================="
echo ""
echo "前端地址: http://127.0.0.1:8001/index.html"
echo "后端地址: http://127.0.0.1:9001"
echo ""
echo "使用说明:"
echo "1. 在浏览器中打开前端地址"
echo "2. 从左侧拖拽节点到画布"
echo "3. 连接节点（点击输出端口 -> 点击输入端口）"
echo "4. 点击 'Save Flow' 保存流程"
echo "5. 点击 'Generate Code' 生成 SDK 代码"
echo ""
echo "按 Ctrl+C 停止服务"
echo "=========================================="

# 等待用户中断
trap "echo ''; echo '正在停止服务...'; kill $BACKEND_PID $FRONTEND_PID 2>/dev/null || true; exit 0" INT TERM

wait
