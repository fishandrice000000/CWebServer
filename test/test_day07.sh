#!/bin/bash
# test_day07.sh — V0.7 多进程 TCP 服务器自动化测试
# 测试: 编译零警告、服务器多进程并发、各路由正确性

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
echo " DAY07 TEST — V0.7 多进程 TCP 服务器"
echo "=============================================="

# ---- 编译 ----
echo ""
echo "[1] 编译 (零警告)"
make clean > /dev/null 2>&1
if make 2>&1 | tee /tmp/day07_build.log | grep -E "warning|error"; then
    red "  FAIL: 编译有警告或错误"
    FAIL=$((FAIL + 1))
else
    green "  PASS: 编译零警告"
    PASS=$((PASS + 1))
fi

# 启动服务器, 处理 5 个客户端后退出
echo ""
echo "[2] 启动服务器 (max_clients=5)"
$SERVER $CONF serve-fork 5 &
SERVER_PID=$!
sleep 1

# ---- 并发请求 ----
echo ""
echo "[3] 并发请求 (5个 curl 同时发送)"

# 清理之前的临时文件
rm -f /tmp/day07_*.out

# 5个并发请求
curl -s http://127.0.0.1:$PORT/hello      > /tmp/day07_1.out 2>&1 &
curl -s http://127.0.0.1:$PORT/users/zhangsan > /tmp/day07_2.out 2>&1 &
curl -s http://127.0.0.1:$PORT/users/lisi     > /tmp/day07_3.out 2>&1 &
curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:$PORT/not-found > /tmp/day07_4.out 2>&1 &
curl -s http://127.0.0.1:$PORT/hello          > /tmp/day07_5.out 2>&1 &

wait

echo ""
echo "[4] 结果验证"

check "/hello"       "Hello, Web!"                          "$(cat /tmp/day07_1.out)"
check "/users/zhangsan" "FOUND zhangsan"                    "$(cat /tmp/day07_2.out)"
check "/users/lisi"     "FOUND lisi"                        "$(cat /tmp/day07_3.out)"
check "/not-found" "404" "$(cat /tmp/day07_4.out)"

check "/hello (2nd)"  "Hello, Web!"                         "$(cat /tmp/day07_5.out)"

# 等待服务器退出 (处理完 5 个客户端后)
echo ""
echo "[5] 等待服务器退出"
wait $SERVER_PID 2>/dev/null || true

# ---- 僵尸进程检查 ----
echo ""
echo "[6] 僵尸进程检查"
ZOMBIES=$(ps aux | awk '$8 ~ /Z/' | wc -l)
if [ "$ZOMBIES" -eq 0 ]; then
    green "  PASS: 无僵尸进程"
    PASS=$((PASS + 1))
else
    red "  FAIL: 发现 $ZOMBIES 个僵尸进程"
    ps aux | awk '$8 ~ /Z/'
    FAIL=$((FAIL + 1))
fi

# ---- SIGPIPE 测试 ----
echo ""
echo "[7] SIGPIPE: 客户端立刻断开"
# 启动服务器, 处理 1 个客户端后退出
$SERVER $CONF serve-fork 1 &
SPID=$!
sleep 1

# 连接后立刻关闭 (不发送任何数据)
timeout 2 bash -c "exec 3<>/dev/tcp/127.0.0.1/$PORT; exec 3>&-" 2>/dev/null || true
sleep 1

# 服务器应该还在运行 (没有被 SIGPIPE 杀死)
if kill -0 $SPID 2>/dev/null; then
    green "  PASS: 服务器未被 SIGPIPE 杀死"
    PASS=$((PASS + 1))
    kill $SPID 2>/dev/null
    wait $SPID 2>/dev/null || true
else
    red "  FAIL: 服务器已崩溃 (可能是 SIGPIPE)"
    FAIL=$((FAIL + 1))
fi

# ---- 总结 ----
echo ""
echo "=============================================="
echo " DAY07 TEST: $PASS PASS, $FAIL FAIL"
echo "=============================================="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
