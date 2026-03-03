#!/bin/bash
# Start FalconMindBuilder Backend

set -e

echo "🚀 Starting FalconMindBuilder Backend..."

# Create virtual environment if not exists
if [ ! -d "venv" ]; then
    echo "📦 Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
echo "🔌 Activating virtual environment..."
source venv/bin/activate

# Install dependencies
echo "📥 Installing dependencies..."
pip install -q -r requirements.txt

# Create .env file if not exists
if [ ! -f ".env" ]; then
    echo "⚙️  Creating .env file..."
    cat > .env << EOF
# FalconMindBuilder Backend Configuration

# Application
APP_NAME=FalconMindBuilder
APP_VERSION=1.0.0
DEBUG=true

# Server
HOST=0.0.0.0
PORT=8000

# Database
DATABASE_URL=sqlite:///./builder.db

# SDK
SDK_PATH=/opt/falconmind/sdk
SDK_ENABLED=false

# MQTT
MQTT_BROKER=localhost
MQTT_PORT=1883
MQTT_ENABLED=false

# CORS
CORS_ORIGINS=["http://localhost:5173","http://localhost:8080","http://127.0.0.1:5173","http://127.0.0.1:8080"]
EOF
fi

# Start server
echo "🌐 Starting server on http://localhost:8000"
echo "📖 API docs: http://localhost:8000/docs"
python -m uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
