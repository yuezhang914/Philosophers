#!/usr/bin/env bash
# ======================================================================
# Philo 自动化测试脚本（适配你的真实代码输出格式）
# - 每个测试都写清：测什么 / 正确结果是什么
# - 全脚本尽量使用 while，避免 for / do-while
# - 依赖：bash, make, grep, awk, sed, timeout(建议)
# ======================================================================

set -u

PHILO_BIN="./philo"
MAKE_CMD="make -s re"
TIMEOUT_CMD="timeout"   # 如果系统没有 timeout，你可以安装 coreutils 或改用 gtimeout

# -------- 小工具：彩色输出（不影响功能）--------
c_green="\033[32m"
c_red="\033[31m"
c_yellow="\033[33m"
c_reset="\033[0m"

pass_count=0
fail_count=0

say() { printf "%b\n" "$*"; }

die() { say "${c_red}[FATAL]${c_reset} $*"; exit 1; }

# -------- 断言工具（失败就记录）--------
ok() {
  pass_count=$((pass_count + 1))
  say "${c_green}[PASS]${c_reset} $*"
}

bad() {
  fail_count=$((fail_count + 1))
  say "${c_red}[FAIL]${c_reset} $*"
}

# 断言：上一条命令退出码 == 0
assert_success() {
  local name="$1"
  local code="$2"
  if [ "$code" -eq 0 ]; then ok "$name"; else bad "$name (exit=$code)"; fi
}

# 断言：上一条命令退出码 != 0
assert_fail() {
  local name="$1"
  local code="$2"
  if [ "$code" -ne 0 ]; then ok "$name"; else bad "$name (exit=$code)"; fi
}

# 断言：文件包含某个字符串
assert_grep() {
  local name="$1"
  local pat="$2"
  local file="$3"
  if grep -qE "$pat" "$file"; then ok "$name"; else bad "$name (not found: $pat)"; fi
}

# 断言：文件不包含某个字符串
assert_no_grep() {
  local name="$1"
  local pat="$2"
  local file="$3"
  if grep -qE "$pat" "$file"; then bad "$name (unexpected: $pat)"; else ok "$name"; fi
}

# 断言：某个模式出现次数 == N
assert_count_eq() {
  local name="$1"
  local pat="$2"
  local file="$3"
  local expected="$4"
  local got
  got="$(grep -cE "$pat" "$file" || true)"
  if [ "$got" -eq "$expected" ]; then ok "$name"; else bad "$name (got=$got expected=$expected)"; fi
}

# -------- 编译 --------
build() {
  say "${c_yellow}==> 1) 编译项目：${MAKE_CMD}${c_reset}"
  $MAKE_CMD || die "编译失败，请先修复编译错误。"
  [ -x "$PHILO_BIN" ] || die "找不到可执行文件 $PHILO_BIN"
  ok "编译成功，找到 $PHILO_BIN"
}

# -------- 运行一次 philo 并保存输出 --------
# 用法：run_case "case_name" "args..." "timeout_seconds"
run_case() {
  local name="$1"
  local args="$2"
  local tsec="$3"
  local out="tests/out/${name}.log"
  local code=0

  mkdir -p tests/out

  # 使用 timeout 防止程序意外挂死（例如死锁导致不退出）
  # 正确项目一般不会超时
  if command -v "$TIMEOUT_CMD" >/dev/null 2>&1; then
    $TIMEOUT_CMD "${tsec}s" $PHILO_BIN $args >"$out" 2>&1
    code=$?
  else
    # 没有 timeout 时，直接运行（可能会卡住；不建议）
    $PHILO_BIN $args >"$out" 2>&1
    code=$?
  fi

  echo "$code" > "tests/out/${name}.code"
  printf "%s" "$out"
}

# -------- 格式检查：每行必须是：timestamp id message --------
# message 必须是 5 种之一：
# "has taken a fork" / "is eating" / "is sleeping" / "is thinking" / "died"
check_log_format() {
  local name="$1"
  local file="$2"
  local n="$3"

  # 用 awk 检查每行字段数、id 范围、消息合法性
  # 通过则退出码 0，否则 1
  awk -v N="$n" '
    BEGIN { ok=1 }
    {
      # 至少 3 列：ts id msg...
      if (NF < 3) { ok=0; exit 1 }
      # timestamp 必须是整数
      if ($1 !~ /^[0-9]+$/) { ok=0; exit 1 }
      # id 必须是整数且 1..N
      if ($2 !~ /^[0-9]+$/) { ok=0; exit 1 }
      if ($2 < 1 || $2 > N) { ok=0; exit 1 }

      # 把第3列到最后拼回 message
      msg=$3
      if (NF >= 4) {
        i=4
        while (i<=NF) { msg=msg" "$i; i++ }
      }

      if (msg != "has taken a fork" &&
          msg != "is eating" &&
          msg != "is sleeping" &&
          msg != "is thinking" &&
          msg != "died") { ok=0; exit 1 }
    }
    END { if (ok==1) exit 0; else exit 1 }
  ' "$file" >/dev/null 2>&1

  if [ $? -eq 0 ]; then ok "$name: 输出格式正确（每行完整且内容合法）"
  else bad "$name: 输出格式不合法（行缺字段/时间戳/id/消息不对）"
  fi
}

# -------- 测试 0：非法参数应失败 --------
test_bad_args() {
  say "${c_yellow}==> 测试 0：参数解析（非法参数应该失败）${c_reset}"

  local out code

  out="$(run_case "bad_args_missing" "" 1)"
  code="$(cat tests/out/bad_args_missing.code)"
  # 正确结果：应该退出非 0，并打印 Error 或类似信息（你代码里是 "Error: bad args"）
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

# -------- 测试 1：1 个哲学家必须最终死亡，且只能打印一次 died --------
test_one_philo_dies() {
  say "${c_yellow}==> 测试 1：1 个哲学家场景（应拿到叉但最终死亡）${c_reset}"
  # 参数：1 die=200ms eat=100 sleep=100
  # 正确结果：
  # - 会出现 "has taken a fork"
  # - 会出现且仅出现一次 "died"
  # - 不会出现 "is eating"（因为拿不到两把叉）
  local out code
  out="$(run_case "one_philo" "1 200 100 100" 2)"
  code="$(cat tests/out/one_philo.code)"

  # timeout 退出码一般是 124（若超时），正常应在 2s 内结束或被监控判死后 stop
  if [ "$code" -eq 124 ]; then bad "1 哲学家：不应超时（可能卡住）"; else ok "1 哲学家：未超时（程序能结束）"; fi

  assert_grep "1 哲学家：应打印拿叉" "has taken a fork" "$out"
  assert_count_eq "1 哲学家：应只死一次" "died$" "$out" 1
  assert_no_grep "1 哲学家：不应出现 eating" "is eating" "$out"
  check_log_format "1 哲学家" "$out" 1
}

# -------- 测试 2：小规模运行一段时间不应超时（基础稳定性）--------
test_basic_stability() {
  say "${c_yellow}==> 测试 2：基础稳定性（多哲学家运行不应卡死）${c_reset}"
  # 参数：4 die=800 eat=200 sleep=200
  # 正确结果：
  # - 程序可能会有人死（也可能不死，取决于调度），但不该卡住
  # - 输出格式必须正确
  local out code
  out="$(run_case "basic_4" "4 800 200 200" 2)"
  code="$(cat tests/out/basic_4.code)"

  if [ "$code" -eq 124 ]; then bad "基础稳定性：超时（可能死锁/无法 stop）"; else ok "基础稳定性：未超时"; fi
  check_log_format "基础稳定性 4" "$out" 4
}

# -------- 测试 3：must_eat 必须让程序自动结束且不出现 died --------
test_must_eat_finish() {
  say "${c_yellow}==> 测试 3：must_eat（吃够次数应自动结束，无 died）${c_reset}"
  # 参数：5 die=800 eat=100 sleep=100 must_eat=3
  # 正确结果：
  # - 最终应退出（不超时）
  # - 不应出现 "died"
  local out code
  out="$(run_case "must_eat_5" "5 800 100 100 3" 3)"
  code="$(cat tests/out/must_eat_5.code)"

  if [ "$code" -eq 124 ]; then bad "must_eat：超时（应自动结束）"; else ok "must_eat：未超时（应自动结束）"; fi
  assert_no_grep "must_eat：不应出现 died" "died$" "$out"
  check_log_format "must_eat 5" "$out" 5
}

# -------- 测试 4：死亡检测应触发（设置很紧的 die_ms）--------
test_death_trigger() {
  say "${c_yellow}==> 测试 4：死亡检测（die_ms 很小应触发 died）${c_reset}"
  # 参数：3 die=150 eat=200 sleep=200
  # 正确结果：
  # - 因为 eat_ms > die_ms，很容易有人在吃/等叉期间超时 -> died
  # - died 应出现 1 次（你 watch 发现死就 stop_set）
  local out code
  out="$(run_case "death_trigger" "3 150 200 200" 2)"
  code="$(cat tests/out/death_trigger.code)"

  if [ "$code" -eq 124 ]; then bad "死亡检测：超时（应在 2s 内死并停止）"; else ok "死亡检测：未超时"; fi
  assert_count_eq "死亡检测：应只打印一次 died" "died$" "$out" 1
  check_log_format "死亡检测 3" "$out" 3
}

# -------- 测试 5：输出行应该“完整一行”，不应出现破碎/乱码 --------
test_output_integrity() {
  say "${c_yellow}==> 测试 5：输出完整性（print_lock 是否有效）${c_reset}"
  # 大一点的并发来冲输出：10 人，持续短时间
  # 正确结果：
  # - 每行都能被格式检查通过（不会出现半行、粘连）
  local out code
  out="$(run_case "output_10" "10 800 100 100" 2)"
  code="$(cat tests/out/output_10.code)"

  if [ "$code" -eq 124 ]; then bad "输出完整性：超时（可能卡住）"; else ok "输出完整性：未超时"; fi
  check_log_format "输出完整性 10" "$out" 10
}

# -------- 主入口：按顺序跑所有测试 --------
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
  say "输出日志在：tests/out/*.log"
  say "=============================================="

  if [ "$fail_count" -ne 0 ]; then
    exit 1
  fi
  exit 0
}

main
