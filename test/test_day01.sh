#!/usr/bin/env bash
set -e

test -f logs/server.log
grep -F "[INFO] server config loaded" logs/server.log

rm -f logs/server.log
rm -f day01_output.txt
rm -f bad_log_output.txt
rm -f config/bad_log.conf

cat > config/bad_log.conf <<'CONFIG'
server_name=BadLog
host=127.0.0.1
port=8080
root=www
log=logs/missing/server.log
CONFIG

make clean
make
./build/mini_web_server config/server.conf > day01_output.txt
grep -F "server_name=MiniWeb" day01_output.txt
grep -F "host=127.0.0.1" day01_output.txt
grep -F "port=8080" day01_output.txt
grep -F "root=www" day01_output.txt
grep -F "HTTP/1.1 200 OK" day01_output.txt
grep -F "Content-Type: text/html" day01_output.txt
grep -F "Hello, Web!" day01_output.txt

set +e
./build/mini_web_server config/bad_log.conf > bad_log_output.txt 2>&1
status=$?
set -e
test "$status" -ne 0
grep -F "failed to open log file" bad_log_output.txt
echo "DAY01 PASS"
