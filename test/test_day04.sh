#!/usr/bin/env bash
set -e

BIN=./build/mini_web_server
CONF=config/server.conf

rm -f outputs/*.out

make clean && make

echo "=== TEST 1: process requests ==="
$BIN $CONF process

echo ""
echo "=== TEST 2: hello.out exists and correct ==="
test -f outputs/hello.out
grep -F "HTTP/1.1 200 OK" outputs/hello.out
grep -F "Hello, Web!" outputs/hello.out

echo ""
echo "=== TEST 3: user_find.out ==="
test -f outputs/user_find.out
grep -F "FOUND zhangsan" outputs/user_find.out

echo ""
echo "=== TEST 4: missing.out ==="
test -f outputs/missing.out
grep -F "HTTP/1.1 404 Not Found" outputs/missing.out

echo ""
echo "=== TEST 5: log contains parent and child ==="
grep -F "process_server: all done" logs/server.log

echo ""
echo "=== TEST 6: program exits cleanly ==="
$BIN $CONF process > /dev/null

echo ""
echo "DAY04 PASS"
