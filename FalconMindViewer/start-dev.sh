#!/bin/bash
# FalconMindViewer 开发环境快速启动脚本
# Usage: ./start-dev.sh

set -e

echo "🚀 FalconMindViewer 开发环境启动脚本"
echo "========================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查命令是否存在
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 检查依赖
echo "📋 检查依赖..."

if ! command_exists docker; then
    echo -e "${RED}❌ Docker 未安装${NC}"
    echo "请访问 https://docs.docker.com/get-docker/ 安装 Docker"
    exit 1
fi

if ! command_exists docker-compose; then
    echo -e "${RED}❌ Docker Compose 未安装${NC}"
    exit 1
fi

if ! command_exists python3; then
    echo -e "${RED}❌ Python3 未安装${NC}"
    exit 1
fi

if ! command_exists node; then
    echo -e "${RED}❌ Node.js 未安装${NC}"
    exit 1
fi

echo -e "${GREEN}✅ 所有依赖已安装${NC}"
echo ""

# 检查是否在项目根目录
if [ ! -f "README.md" ] || [ ! -d "backend" ] || [ ! -d "frontend" ]; then
    echo -e "${RED}❌ 请在项目根目录运行此脚本${NC}"
    exit 1
fi

PROJECT_ROOT=$(pwd)
echo "📂 项目根目录: $PROJECT_ROOT"
echo ""

# 启动基础设施
echo "🐳 启动基础设施 (PostgreSQL, Redis)..."
docker-compose -f docker-compose.dev.yml up -d postgres redis

# 等待数据库就绪
echo "⏳ 等待数据库就绪..."
sleep 5

# 检查数据库连接
if docker-compose -f docker-compose.dev.yml exec -T postgres pg_isready -U falconmind >/dev/null 2>&1; then
    echo -e "${GREEN}✅ PostgreSQL 已就绪${NC}"
else
    echo -e "${YELLOW}⚠️ PostgreSQL 可能尚未就绪，稍后再试${NC}"
fi

echo ""

# 后端设置
echo "🐍 设置后端环境..."
cd backend

# 创建虚拟环境
if [ ! -d "venv" ]; then
    echo "📦 创建 Python 虚拟环境..."
    python3 -m venv venv
fi

# 激活虚拟环境
source venv/bin/activate

# 安装依赖
echo "📦 安装后端依赖..."
pip install -q -r requirements.txt

# 数据库迁移
echo "🗄️  执行数据库迁移..."
alembic upgrade head

# 初始化数据
echo "🌱 初始化数据..."
python scripts/init_data.py

echo ""
echo -e "${GREEN}✅ 后端环境设置完成${NC}"
echo ""

# 前端设置
echo "⚛️  设置前端环境..."
cd ../frontend

# 安装依赖
if [ ! -d "node_modules" ]; then
    echo "📦 安装前端依赖..."
    npm install
else
    echo "📦 前端依赖已安装"
fi

echo ""
echo -e "${GREEN}✅ 前端环境设置完成${NC}"
echo ""

cd "$PROJECT_ROOT"

# 创建启动脚本
cat > start-services.sh << 'EOF'
#!/bin/bash
# 启动所有服务

echo "🚀 启动 FalconMindViewer 开发服务..."
echo ""

# 启动后端
echo "🐍 启动后端服务..."
cd backend
source venv/bin/activate
uvicorn app.main:app --reload --host 0.0.0.0 --port 9000 &
BACKEND_PID=$!
echo "后端 PID: $BACKEND_PID"
cd ..

# 启动前端
echo "⚛️  启动前端服务..."
cd frontend
npm run dev &
FRONTEND_PID=$!
echo "前端 PID: $FRONTEND_PID"
cd ..

echo ""
echo "========================================"
echo "✅ 所有服务已启动!"
echo ""
echo "📱 前端地址: http://localhost:8080"
echo "🔌 后端地址: http://localhost:9000"
echo "📚 API文档:  http://localhost:9000/docs"
echo ""
echo "🛑 停止服务: kill $BACKEND_PID $FRONTEND_PID"
echo "========================================"

# 等待用户输入
read -p "按 Enter 键停止所有服务..."

kill $BACKEND_PID $FRONTEND_PID 2>/dev/null
echo "✅ 服务已停止"
EOF

chmod +x start-services.sh

# 输出完成信息
echo ""
echo "========================================"
echo -e "${GREEN}🎉 开发环境设置完成!${NC}"
echo "========================================"
echo ""
echo "📋 下一步操作:"
echo ""
echo "1️⃣  启动所有服务:"
echo "   ./start-services.sh"
echo ""
echo "2️⃣  单独启动后端:"
echo "   cd backend"
echo "   source venv/bin/activate"
echo "   uvicorn app.main:app --reload"
echo ""
echo "3️⃣  单独启动前端:"
echo "   cd frontend"
echo "   npm run dev"
echo ""
echo "📖 文档:"
echo "   - 项目说明:    README.md"
echo "   - 任务清单:    TODO.md"
echo "   - 详细设计:    docs/architecture/system-design-v1.md"
echo "   - API文档:     docs/api/api-reference.md"
echo "   - 部署指南:    docs/deployment/deployment-guide.md"
echo ""
echo "🐳 Docker 命令:"
echo "   # 查看服务状态"
echo "   docker-compose -f docker-compose.dev.yml ps"
echo ""
echo "   # 查看日志"
echo "   docker-compose -f docker-compose.dev.yml logs -f"
echo ""
echo "   # 停止基础设施"
echo "   docker-compose -f docker-compose.dev.yml down"
echo ""
echo "========================================"
