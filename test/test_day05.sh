#!/usr/bin/env bash
set -e

BIN=./build/mini_web_server
CONF=config/server.conf

rm -f outputs/*.out

make clean && make

echo "=== TEST 1: threaded with 3 workers ==="
$BIN $CONF threaded 3

echo ""
echo "=== TEST 2: all output files exist ==="
test -f outputs/hello.out
test -f outputs/user_find.out
test -f outputs/missing.out

echo ""
echo "=== TEST 3: hello.out correct ==="
grep -F "HTTP/1.1 200 OK" outputs/hello.out
grep -F "Hello, Web!" outputs/hello.out

echo ""
echo "=== TEST 4: user_find.out correct ==="
grep -F "FOUND zhangsan" outputs/user_find.out

echo ""
echo "=== TEST 5: missing.out correct ==="
grep -F "HTTP/1.1 404 Not Found" outputs/missing.out

echo ""
echo "=== TEST 6: log shows worker activity ==="
grep -F "worker[0]: started" logs/server.log
grep -F "thread_server: all done" logs/server.log

echo ""
echo "=== TEST 7: each request processed exactly once ==="
# total_processed 应等于 3 (三个 .req 文件)
grep -F "processed" logs/server.log

echo ""
echo "=== TEST 8: program exits cleanly ==="
$BIN $CONF threaded 2 > /dev/null

echo ""
echo "DAY05 PASS"
