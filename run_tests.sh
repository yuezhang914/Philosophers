#!/usr/bin/env bash

# ======================================================================

# Philo 自动化测试脚本（V4：避免反引号/引号地狱的稳定版）

# - 不使用 `...` 反引号命令替换

# - stdout/stderr 分离

# - 必结束用例用 must_eat

# - 格式检查：awk 输出错误原因到固定临时文件（不用变量重定向）

# ======================================================================

set -u

PHILO_BIN="./philo"
MAKE_CMD="make -s re"
TIMEOUT_CMD="timeout"

c_green="\033[32m"
c_red="\033[31m"
c_yellow="\033[33m"
c_reset="\033[0m"

pass_count=0
fail_count=0

say() { printf "%b\n" "$*"; }
die() { say "${c_red}[FATAL]${c_reset} $*"; exit 1; }
ok() { pass_count=$((pass_count + 1)); say "${c_green}[PASS]${c_reset} $*"; }
bad() { fail_count=$((fail_count + 1)); say "${c_red}[FAIL]${c_reset} $*"; }

assert_fail() {
local name="$1"
local code="$2"
if [ "$code" -ne 0 ]; then ok "$name"; else bad "$name (exit=$code)"; fi
}

assert_grep() {
local name="$1"
local pat="$2"
local file="$3"
if grep -qE "$pat" "$file"; then ok "$name"; else bad "$name (not found: $pat)"; fi
}

assert_no_grep() {
local name="$1"
local pat="$2"
local file="$3"
if grep -qE "$pat" "$file"; then bad "$name (unexpected: $pat)"; else ok "$name"; fi
}

assert_count_eq() {
local name="$1"
local pat="$2"
local file="$3"
local expected="$4"
local got
got="$(grep -cE "$pat" "$file" || true)"
if [ "$got" -eq "$expected" ]; then ok "$name"; else bad "$name (got=$got expected=$expected)"; fi
}

build() {
say "${c_yellow}==> 1) 编译项目：${MAKE_CMD}${c_reset}"
$MAKE_CMD || die "编译失败，请先修复编译错误。"
[ -x "$PHILO_BIN" ] || die "找不到可执行文件 $PHILO_BIN"
ok "编译成功，找到 $PHILO_BIN"
}

run_case() {
local name="$1"
local args="$2"
local tsec="$3"
local out="tests/out/${name}.log"
local err="tests/out/${name}.err"
local code=0

mkdir -p tests/out

if command -v "$TIMEOUT_CMD" >/dev/null 2>&1; then
$TIMEOUT_CMD "${tsec}s" $PHILO_BIN $args >"$out" 2>"$err"
code=$?
else
$PHILO_BIN $args >"$out" 2>"$err"
code=$?
fi

echo "$code" > "tests/out/${name}.code"
printf "%s" "$out"
}

# 格式检查：

# - 去掉 \r

# - 跳过空行

# - 每行必须：ts id msg

# - msg 只能是 5 种之一

# - 若失败，把“第一条坏行原因”写到 tests/out/_bad_reason.txt

check_log_format() {
local name="$1"
local file="$2"
local n="$3"
local reason="tests/out/_bad_reason.txt"

: > "$reason"

awk -v N="$n" -v REASON="$reason" '
function trim(s) { sub(/^[ \t]+/, "", s); sub(/[ \t]+$/, "", s); return s }
{
gsub(/\r/, "", $0)
line=$0
if (line ~ /^[ \t]*$/) next

```
  # 前缀：ts id 必须是数字
  if (line !~ /^[0-9]+[ \t]+[0-9]+[ \t]+/) {
    print "bad prefix: [" line "]" > REASON
    exit 1
  }

  ts=$1
  id=$2
  if (ts !~ /^[0-9]+$/) {
    print "bad ts: [" line "]" > REASON
    exit 1
  }
  if (id !~ /^[0-9]+$/) {
    print "bad id: [" line "]" > REASON
    exit 1
  }
  if (id < 1 || id > N) {
    print "id out of range: [" line "] (id=" id ", N=" N ")" > REASON
    exit 1
  }

  # msg：删掉前缀 ts+空白+id+空白
  msg=line
  sub(/^[0-9]+[ \t]+[0-9]+[ \t]+/, "", msg)
  msg=trim(msg)

  if (msg !~ /^(has taken a fork|is eating|is sleeping|is thinking|died)$/) {
    print "bad msg: [" line "] (msg=[" msg "])" > REASON
    exit 1
  }
}
END { exit 0 }
```

' "$file" >/dev/null 2>&1

if [ $? -eq 0 ]; then
ok "$name: 输出格式正确（每行完整且内容合法）"
else
bad "$name: 输出格式不合法（行缺字段/时间戳/id/消息不对）"
if [ -s "$reason" ]; then
say "  -> 第一条坏因：$(head -n 1 "$reason")"
else
say "  -> 未捕获原因：请贴 $file 的前 5 行"
fi
say "  -> 查看日志：$file 以及同名 .err"
fi
}

test_bad_args() {
say "${c_yellow}==> 测试 0：参数解析（非法参数应该失败）${c_reset}"
local out code

out="$(run_case "bad_args_missing" "" 1)"
code="$(cat tests/out/bad_args_missing.code)"
assert_fail "缺参数应失败" "$code"

out="$(run_case "bad_args_nonnumeric" "a 100 100 100" 1)"
code="$(cat tests/out/bad_args_nonnumeric.code)"
assert_fail "非数字参数应失败" "$code"

out="$(run_case "bad_args_zero" "0 100 100 100" 1)"
code="$(cat tests/out/bad_args_zero.code)"
assert_fail "哲学家数量为 0 应失败" "$code"

out="$(run_case "bad_args_overflow" "2147483648 100 100 100" 1)"
code="$(cat tests/out/bad_args_overflow.code)"
assert_fail "超过 INT_MAX 应失败" "$code"
}

test_one_philo_dies() {
say "${c_yellow}==> 测试 1：1 个哲学家场景（应拿到叉但最终死亡）${c_reset}"
local out code

out="$(run_case "one_philo" "1 200 100 100" 2)"
code="$(cat tests/out/one_philo.code)"

if [ "$code" -eq 124 ]; then bad "1 哲学家：不应超时（可能卡住）"; else ok "1 哲学家：未超时（程序能结束）"; fi

assert_grep "1 哲学家：应打印拿叉" "has taken a fork" "$out"
assert_count_eq "1 哲学家：应只死一次" "died$" "$out" 1
assert_no_grep "1 哲学家：不应出现 eating" "is eating" "$out"

check_log_format "1 哲学家" "$out" 1
}

test_basic_stability() {
say "${c_yellow}==> 测试 2：基础稳定性（多哲学家运行应可正常结束）${c_reset}"
local out code

out="$(run_case "basic_4" "4 800 200 200 3" 3)"
code="$(cat tests/out/basic_4.code)"

if [ "$code" -eq 124 ]; then bad "基础稳定性：超时（应能自然结束）"; else ok "基础稳定性：未超时（能自然结束）"; fi
check_log_format "基础稳定性 4" "$out" 4
}

test_must_eat_finish() {
say "${c_yellow}==> 测试 3：must_eat（吃够次数应自动结束，无 died）${c_reset}"
local out code

out="$(run_case "must_eat_5" "5 800 100 100 3" 3)"
code="$(cat tests/out/must_eat_5.code)"

if [ "$code" -eq 124 ]; then bad "must_eat：超时（应自动结束）"; else ok "must_eat：未超时（应自动结束）"; fi

assert_no_grep "must_eat：不应出现 died" "died$" "$out"
check_log_format "must_eat 5" "$out" 5
}

test_death_trigger() {
say "${c_yellow}==> 测试 4：死亡检测（die_ms 很小应触发 died）${c_reset}"
local out code

out="$(run_case "death_trigger" "3 150 200 200" 2)"
code="$(cat tests/out/death_trigger.code)"

if [ "$code" -eq 124 ]; then bad "死亡检测：超时（应在 2s 内死并停止）"; else ok "死亡检测：未超时"; fi

assert_count_eq "死亡检测：应只打印一次 died" "died$" "$out" 1
check_log_format "死亡检测 3" "$out" 3
}

test_output_integrity() {
say "${c_yellow}==> 测试 5：输出完整性（print_lock 是否有效）${c_reset}"
local out code

out="$(run_case "output_10" "10 800 100 100 2" 4)"
code="$(cat tests/out/output_10.code)"

if [ "$code" -eq 124 ]; then bad "输出完整性：超时（应能自然结束）"; else ok "输出完整性：未超时（能自然结束）"; fi
check_log_format "输出完整性 10" "$out" 10
}

main() {
build
test_bad_args
test_one_philo_dies
test_basic_stability
test_must_eat_finish
test_death_trigger
test_output_integrity

say ""
say "================== 结果汇总 =================="
say "${c_green}PASS${c_reset}: $pass_count"
say "${c_red}FAIL${c_reset}: $fail_count"
say "日志：tests/out/*.log（只含 stdout）"
say "错误：tests/out/*.err（stderr）"
say "=============================================="

if [ "$fail_count" -ne 0 ]; then exit 1; fi
exit 0
}

main

