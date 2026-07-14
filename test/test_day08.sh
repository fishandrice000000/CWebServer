#!/bin/bash
# test_day08.sh — V0.8 线程池 TCP 服务器自动化测试
# 测试: 编译零警告、线程池并发、各路由正确性

set -e

SERVER="./build/mini_web_server"
CONF="config/server.conf"
PORT=8080
PASS=0
FAIL=0

green() { echo -e "\033[32m$*\033[0m"; }
red()   { echo -e "\033[31m$*\033[0m"; }

check() {
    local desc="$1"
    local expected="$2"
    local actual="$3"
    if echo "$actual" | grep -qF "$expected"; then
        green "  PASS: $desc"
        PASS=$((PASS + 1))
    else
        red "  FAIL: $desc"
        red "    expected: $expected"
        red "    got:      $actual"
        FAIL=$((FAIL + 1))
    fi
}

echo "=============================================="
echo " DAY08 TEST — V0.8 线程池 TCP 服务器"
echo "=============================================="

# ---- 编译 ----
echo ""
echo "[1] 编译 (零警告)"
make clean > /dev/null 2>&1
if make 2>&1 | tee /tmp/day08_build.log | grep -E "warning|error"; then
    red "  FAIL: 编译有警告或错误"
    FAIL=$((FAIL + 1))
else
    green "  PASS: 编译零警告"
    PASS=$((PASS + 1))
fi

# 启动服务器, 处理 5 个客户端后退出, 3 个 worker
echo ""
echo "[2] 启动服务器 (workers=3, max_clients=5)"
$SERVER $CONF serve-pool 3 5 &
SERVER_PID=$!
sleep 1

# ---- 并发请求 ----
echo ""
echo "[3] 并发请求 (5个 curl 同时发送)"

rm -f /tmp/day08_*.out

curl -s http://127.0.0.1:$PORT/hello           > /tmp/day08_1.out 2>&1 &
curl -s http://127.0.0.1:$PORT/users/zhangsan      > /tmp/day08_2.out 2>&1 &
curl -s http://127.0.0.1:$PORT/users/lisi          > /tmp/day08_3.out 2>&1 &
curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:$PORT/not-found > /tmp/day08_4.out 2>&1 &
curl -s http://127.0.0.1:$PORT/hello           > /tmp/day08_5.out 2>&1 &

wait

echo ""
echo "[4] 结果验证"

check "/hello"       "Hello, Web!"       "$(cat /tmp/day08_1.out)"
check "/users/zhangsan" "FOUND zhangsan" "$(cat /tmp/day08_2.out)"
check "/users/lisi"     "FOUND lisi"     "$(cat /tmp/day08_3.out)"
check "/not-found"   "404"               "$(cat /tmp/day08_4.out)"
check "/hello (2nd)" "Hello, Web!"       "$(cat /tmp/day08_5.out)"

# 等待服务器退出
echo ""
echo "[5] 等待服务器退出"
wait $SERVER_PID 2>/dev/null || true

# ---- SIGPIPE 测试 ----
echo ""
echo "[6] SIGPIPE: 客户端立刻断开"
$SERVER $CONF serve-pool 2 2 &
SPID=$!
sleep 1

timeout 2 bash -c "exec 3<>/dev/tcp/127.0.0.1/$PORT; exec 3>&-" 2>/dev/null || true
sleep 1

if kill -0 $SPID 2>/dev/null; then
    green "  PASS: 服务器未被 SIGPIPE 杀死"
    PASS=$((PASS + 1))
    curl -s http://127.0.0.1:$PORT/hello > /dev/null
    wait $SPID 2>/dev/null || true
else
    red "  FAIL: 服务器已崩溃"
    FAIL=$((FAIL + 1))
fi

# ---- 总结 ----
echo ""
echo "=============================================="
echo " DAY08 TEST: $PASS PASS, $FAIL FAIL"
echo "=============================================="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
