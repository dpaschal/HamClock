#!/bin/bash
# Phase 2 Data Fetching Test

echo "=== Phase 2: Async Data Fetching Test ==="
echo ""

echo "[1] Testing Space Weather API (spacex.land)..."
curl -s "https://api.spacex.land/now/kp" | jq . | head -10
echo ""

echo "[2] Testing Weather Forecast API (Open-Meteo)..."
curl -s "https://api.open-meteo.com/v1/forecast?latitude=51.48&longitude=-0.00&hourly=temperature_2m" | jq '.hourly.time[0:3]' 2>/dev/null
echo ""

echo "[3] Running HamClock with async fetching..."
timeout 10 /tmp/HamClock/rust-src/target/release/hamclock 2>&1 | grep -E "weather|forecast|satellite|space|DX|Fetched|Fetching|Updated" || true
echo ""

echo "=== Phase 2 Test Complete ==="
