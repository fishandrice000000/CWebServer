#!/usr/bin/env bash
set -e

BIN=./build/mini_web_server
CONF=config/server.conf

make clean && make

echo "=== TEST 1: index (inorder output) ==="
$BIN $CONF index

echo ""
echo "=== TEST 2: find-index existing user ==="
$BIN $CONF find-index zhangsan | grep -F "FOUND"

echo ""
echo "=== TEST 3: find-index non-existing user ==="
$BIN $CONF find-index nobody | grep -F "NOT_FOUND"

echo ""
echo "=== TEST 4: compare (list vs tree steps) ==="
$BIN $CONF compare zhangsan | grep -F "list search"
$BIN $CONF compare zhangsan | grep -F "tree search"

echo ""
echo "=== TEST 5: compare non-existing ==="
$BIN $CONF compare nobody | grep -F "NOT_FOUND"

echo ""
echo "=== TEST 6: program exits cleanly ==="
$BIN $CONF index > /dev/null

echo ""
echo "DAY03 PASS"
