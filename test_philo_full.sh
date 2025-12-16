#!/bin/sh

# 全量测试脚本：覆盖 A~E + 额外两项（叉子正确性统计 + valgrind 泄漏）
# 用法：
#   chmod +x test_philo_full.sh
#   ./test_philo_full.sh
#
# 输出日志：./test_out/*.log

BIN="./philo"
OUT_DIR="./test_out"
mkdir -p "$OUT_DIR"

print_head() {
  printf "\n========================================\n"
  printf "%s\n" "$1"
  printf "========================================\n"
}

# 运行一个用例：输出保存到文件，打印退出码和前10行
run_case() {
  name="$1"
  cmd="$2"
  out="$OUT_DIR/$name.log"

  print_head "$name"
  printf "【运行】%s\n" "$cmd"
  printf "【日志】%s\n\n" "$out"

  sh -c "$cmd" >"$out" 2>&1
  code=$?

  printf "【退出码】%s\n" "$code"
  printf "【前10行预览】\n"
  head -n 10 "$out"
  printf "\n"
  return 0
}

# 带超时运行：防止死锁/卡死
# 优先用 timeout；没有就用后台+轮询方式
run_case_timeout() {
  name="$1"
  cmd="$2"
  sec="$3"
  out="$OUT_DIR/$name.log"

  print_head "$name"
  printf "【运行(超时%ss)】%s\n" "$sec" "$cmd"
  printf "【日志】%s\n\n" "$out"

  if command -v timeout >/dev/null 2>&1; then
    timeout "$sec" sh -c "$cmd" >"$out" 2>&1
    code=$?
  else
    sh -c "$cmd" >"$out" 2>&1 &
    pid=$!
    t=0
    code=0
    while kill -0 "$pid" 2>/dev/null; do
      if [ "$t" -ge "$sec" ]; then
        printf "【超时】杀掉进程 pid=%s\n" "$pid" >>"$out"
        kill "$pid" 2>/dev/null
        code=124
        break
      fi
      sleep 1
      t=$((t + 1))
    done
    if [ "$code" -ne 124 ]; then
      wait "$pid"
      code=$?
    fi
  fi

  printf "【退出码】%s (124 通常表示超时)\n" "$code"
  printf "【前10行预览】\n"
  head -n 10 "$out"
  printf "\n"
  return 0
}

# C：检查每行日志格式是否正确（时间 id 状态）
check_lines_format() {
  file="$1"
  bad="$(grep -nEv '^[0-9]+ [0-9]+ (has taken a fork|is eating|is sleeping|is thinking|died)$' "$file" || true)"
  if [ -n "$bad" ]; then
    printf "❌ 格式错误行（行号:内容）：\n%s\n" "$bad"
    return 1
  fi
  printf "✅ 日志每行格式正确\n"
  return 0
}

# C：检查 died 后是否还有其他输出（应尽快停止刷屏）
check_stop_after_died() {
  file="$1"
  line="$(grep -n ' died$' "$file" | head -n 1 | cut -d: -f1)"
  if [ -z "$line" ]; then
    printf "⚠️ 没找到 died 行（这个用例可能没人死）\n"
    return 0
  fi
  tail_part="$(tail -n +"$((line + 1))" "$file" | head -n 20 | grep -E ' (is eating|is sleeping|is thinking|has taken a fork)$' || true)"
  if [ -n "$tail_part" ]; then
    printf "❌ died 之后仍出现状态输出（展示 died 后最多20行里命中的内容）：\n%s\n" "$tail_part"
    return 1
  fi
  printf "✅ died 后没有继续刷状态（至少在 died 后20行内）\n"
  return 0
}

# B：单哲学家 died 时间是否“接近 die_ms”（粗略检查10ms要求）
# 由于调度存在误差，这里给一个宽容范围：[-5ms, +15ms]
check_single_die_delay() {
  file="$1"
  die_ms="$2"
  ts="$(awk '$2==1 && $3=="died"{print $1; exit}' "$file")"
  if [ -z "$ts" ]; then
    printf "❌ 单哲学家日志里没找到 died\n"
    return 1
  fi
  low=$((die_ms - 5))
  high=$((die_ms + 15))
  if [ "$ts" -lt "$low" ] || [ "$ts" -gt "$high" ]; then
    printf "⚠️ died 时间戳=%s，不在预期范围 [%s, %s]\n" "$ts" "$low" "$high"
    printf "   说明：这不是绝对判死刑，但提示你关注“监控频率/睡眠精度”。\n"
    return 1
  fi
  printf "✅ 单哲学家 died 时间戳=%s，接近 die_ms=%s（粗略通过）\n" "$ts" "$die_ms"
  return 0
}

# 额外：叉子正确性间接检测（序列统计）
# 规则：同一个哲学家每次出现 "is eating" 之前，应该已经打印过两次 "has taken a fork"
# 并且在一次吃饭周期里，"has taken a fork" 不应超过 2 次
check_fork_sequence() {
  file="$1"
  awk '
    {
      id=$2;
      if ($3=="has") take[id]++
      if ($3=="is" && $4=="eating") {
        if (take[id] != 2) {
          printf("❌ 哲学家 %d 在 eating 前拿叉次数=%d（应为2）\n", id, take[id]);
          bad=1;
        }
        take[id]=0;
      }
      if (take[id] > 2) {
        printf("❌ 哲学家 %d 连续拿叉次数超过2（=%d）\n", id, take[id]);
        bad=1;
        take[id]=0;
      }
    }
    END {
      if (bad!=1) print "✅ 叉子序列统计通过：每次 eating 前都有2次拿叉";
      exit bad;
    }
  ' "$file"
}

# D：helgrind 检查（如果系统有 valgrind）
run_helgrind() {
  name="$1"
  cmd="$2"
  out="$OUT_DIR/$name.helgrind.log"

  print_head "$name"
  if ! command -v valgrind >/dev/null 2>&1; then
    printf "⚠️ 未安装 valgrind，跳过 helgrind\n"
    return 0
  fi
  printf "【运行】valgrind --tool=helgrind %s\n" "$cmd"
  printf "【日志】%s\n\n" "$out"
  valgrind --tool=helgrind --quiet sh -c "$cmd" >"$out" 2>&1
  printf "【提示】打开日志搜索：\"Possible data race\" / \"race\" / \"lock order\"。\n"
  printf "【预期】不应出现数据竞争报告。\n"
}

# A：最严格 valgrind 泄漏检查（按你要求的参数）
run_valgrind_leak() {
  name="$1"
  cmd="$2"
  out="$OUT_DIR/$name.valgrind.log"

  print_head "$name"
  if ! command -v valgrind >/dev/null 2>&1; then
    printf "⚠️ 未安装 valgrind，跳过泄漏检查\n"
    return 0
  fi
  printf "【运行】最严格泄漏检查（可能会比较慢）\n"
  printf "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes %s\n" "$cmd"
  printf "【日志】%s\n\n" "$out"
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    sh -c "$cmd" >"$out" 2>&1
  printf "【预期】日志里应看到：\"definitely lost: 0 bytes\"。\n"
}

# ---------- 开始执行 ----------

print_head "Step 0: make re"
make re

if [ ! -x "$BIN" ]; then
  printf "❌ 找不到可执行文件：%s\n" "$BIN"
  exit 1
fi
printf "✅ OK: %s\n" "$BIN"

# ========== A 基础参数测试 ==========
run_case "A1_argc_too_few" "$BIN 5 800 200"
printf "【测什么】参数不足\n【期望】退出码非0；输出 Error\n"

run_case "A2_argc_too_many" "$BIN 5 800 200 200 7 99"
printf "【测什么】参数过多\n【期望】退出码非0；输出 Error\n"

run_case "A3_not_number" "$BIN 5 800 abc 200"
printf "【测什么】非数字\n【期望】退出码非0；输出 Error\n"

run_case "A4_zero" "$BIN 0 800 200 200"
printf "【测什么】0 参数\n【期望】退出码非0；输出 Error\n"

run_case "A5_negative" "$BIN -5 800 200 200"
printf "【测什么】负数参数\n【期望】退出码非0；输出 Error\n"

# ========== B 边界与极限 ==========
run_case_timeout "B1_one_philo_must_die" "$BIN 1 300 100 100" 3
printf "【测什么】1个人必死边界\n【期望】出现 '1 died'；程序在3秒内结束\n"
check_single_die_delay "$OUT_DIR/B1_one_philo_must_die.log" 300
check_stop_after_died "$OUT_DIR/B1_one_philo_must_die.log"

run_case_timeout "B2_two_philo_no_deadlock" "$BIN 2 1000 200 200 5" 6
printf "【测什么】2个人最容易卡死/死锁\n【期望】应在6秒内结束（因为 must_eat=5）；不应超时(124)\n"

run_case_timeout "B3_tight_time_should_die" "$BIN 5 210 200 50" 4
printf "【测什么】极限时间：die 很小，容易死\n【期望】一般会出现 died；且 died 后不再继续刷状态\n"
check_stop_after_died "$OUT_DIR/B3_tight_time_should_die.log"

# ========== C 日志规则 ==========
print_head "C1_log_format_check (对 B3 日志做格式检查)"
check_lines_format "$OUT_DIR/B3_tight_time_should_die.log"
printf "【测什么】日志每行格式 + 是否乱码\n【期望】所有行匹配：时间 id 状态\n"

# ========== E must_eat 停止条件 ==========
run_case_timeout "E1_must_eat_finish" "$BIN 3 2000 200 200 2" 5
printf "【测什么】must_eat 停止条件\n【期望】应在5秒内结束；通常没有 died\n"

# ========== 额外：叉子正确性间接检测 ==========
print_head "X1_fork_sequence_check (对 E1 日志做拿叉-吃饭序列统计)"
check_fork_sequence "$OUT_DIR/E1_must_eat_finish.log"
printf "【测什么】间接验证：每次 eating 前是否有两次拿叉\n【期望】每个哲学家都满足：2次 fork -> eating\n"

# ========== D 并发正确性（helgrind） ==========
run_helgrind "D1_helgrind_race_check" "$BIN 3 2000 200 200 2"

# ========== A 内存泄漏（最严格 valgrind） ==========
run_valgrind_leak "A6_valgrind_leak_check" "$BIN 3 2000 200 200 2"

print_head "All done"
printf "日志目录：%s\n" "$OUT_DIR"
printf "建议你重点看：\n"
printf "  - %s/B2_two_philo_no_deadlock.log（是否超时/卡死）\n" "$OUT_DIR"
printf "  - %s/B3_tight_time_should_die.log（died 后是否还刷）\n" "$OUT_DIR"
printf "  - %s/A6_valgrind_leak_check.valgrind.log（definitely lost 是否为0）\n" "$OUT_DIR"
printf "  - %s/D1_helgrind_race_check.helgrind.log（是否有 data race 报告）\n" "$OUT_DIR"
