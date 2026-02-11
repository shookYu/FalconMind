#!/bin/bash
# NodeAgent 测试脚本

set -e

cd "$(dirname "$0")/../build"

echo "=========================================="
echo "NodeAgent 功能测试"
echo "=========================================="
echo ""

# 清理旧进程
pkill -f cluster_center_mock 2>/dev/null || true
sleep 1

echo "测试 1: Telemetry 上行流（优化后的 JSON 格式）"
echo "----------------------------------------"
./cluster_center_mock 8888 > /tmp/mock_test1.log 2>&1 &
MOCK_PID=$!
sleep 2

timeout 4 ./test_telemetry_flow 127.0.0.1 8888 2>&1 || true
sleep 1
kill $MOCK_PID 2>/dev/null || true
sleep 1

echo ""
echo "Cluster Center Mock 接收到的消息（优化格式）："
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
grep -A 10 "Received Telemetry" /tmp/mock_test1.log | head -40 || echo "未接收到消息"
echo ""

echo ""
echo "测试 2: 下行消息接收（Command/Mission）"
echo "----------------------------------------"
echo "注意：此测试需要手动交互"
echo "1. 在一个终端运行: ./cluster_center_mock 8888"
echo "2. 在另一个终端运行: ./test_downlink_demo 127.0.0.1 8888"
echo "3. 在 Cluster Center Mock 终端输入: send CMD:{\"type\":\"ARM\"}"
echo ""

echo "测试完成！"
echo "=========================================="
