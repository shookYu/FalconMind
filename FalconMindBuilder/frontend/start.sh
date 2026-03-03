#!/bin/bash
# Start FalconMindBuilder Frontend

set -e

echo "🚀 Starting FalconMindBuilder Frontend..."

# Install dependencies if node_modules doesn't exist
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    pnpm install
fi

# Start dev server
echo "🌐 Starting dev server on http://localhost:5173"
pnpm dev
