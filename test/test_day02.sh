#!/usr/bin/env bash
set -e

BIN=./build/mini_web_server
CONF=config/server.conf

make clean && make

echo "=== TEST 1: list ==="
$BIN $CONF list

echo ""
echo "=== TEST 2: find existing user ==="
$BIN $CONF find zhangsan | grep -F "FOUND"

echo ""
echo "=== TEST 3: find non-existing user ==="
$BIN $CONF find nobody | grep -F "NOT_FOUND"

echo ""
echo "=== TEST 4: add new user ==="
$BIN $CONF add testuser testpass 13900000001 | grep -F "ADD_OK"
$BIN $CONF find testuser | grep -F "FOUND"

echo ""
echo "=== TEST 5: add duplicate user ==="
$BIN $CONF add testuser anotherpass 13900000002 | grep -F "ADD_FAILED"

echo ""
echo "=== TEST 6: delete user ==="
$BIN $CONF delete testuser | grep -F "DELETE_OK"
$BIN $CONF find testuser | grep -F "NOT_FOUND"

echo ""
echo "=== TEST 7: delete non-existing user ==="
$BIN $CONF delete nobody | grep -F "DELETE_FAILED"

echo ""
echo "DAY02 PASS"
