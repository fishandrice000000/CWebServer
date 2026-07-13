#!/usr/bin/env bash
set -e

BIN=./build/mini_web_server
CONF=config/server.conf
HOST=127.0.0.1
PORT=8080

make clean && make

echo "=== TEST 1: GET /hello ==="
$BIN $CONF serve &
PID=$! && sleep 1
curl -s http://${HOST}:${PORT}/hello | grep -F "Hello, Web!"
wait $PID
    sleep 0.5

echo ""
echo "=== TEST 2: HTTP 200 status ==="
$BIN $CONF serve &
PID=$! && sleep 1
STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://${HOST}:${PORT}/hello)
echo "status: $STATUS"
test "$STATUS" = "200"
wait $PID
    sleep 0.5

echo ""
echo "=== TEST 3: GET /users/zhangsan ==="
$BIN $CONF serve &
PID=$! && sleep 1
curl -s http://${HOST}:${PORT}/users/zhangsan | grep -F "FOUND zhangsan"
wait $PID
    sleep 0.5

echo ""
echo "=== TEST 4: HTTP 404 ==="
$BIN $CONF serve &
PID=$! && sleep 1
STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://${HOST}:${PORT}/not-exist)
echo "status: $STATUS"
test "$STATUS" = "404"
wait $PID
    sleep 0.5

echo ""
echo "=== TEST 5: NOT_FOUND ==="
$BIN $CONF serve &
PID=$! && sleep 1
curl -s http://${HOST}:${PORT}/users/nobody | grep -F "NOT_FOUND nobody"
wait $PID
    sleep 0.5

echo ""
echo "DAY06 PASS"
