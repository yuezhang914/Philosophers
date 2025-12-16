#!/bin/sh

# 简单测试脚本：用 printf 说明每个测试在测什么、期望什么结果
# 用法：./test_philo.sh

BIN="./philo"
OUT_DIR="./test_out"

mkdir -p "$OUT_DIR"

print_head() {
  printf "\n========================================\n"
  printf "%s\n" "$1"
  printf "========================================\n"
}

run_case() {
  name="$1"
  cmd="$2"
  out="$OUT_DIR/$name.log"

  print_head "$name"
  printf "运行命令：%s\n" "$cmd"
  printf "输出保存：%s\n\n" "$out"

  # 运行并记录输出
  sh -c "$cmd" >"$out" 2>&1
  code=$?

  printf "退出码（0表示正常结束，非0表示报错或失败）：%s\n" "$code"
  printf "输出前10行预览：\n"
  head -n 10 "$out"
  printf "\n"
  return 0
}


check_lines_format() {
  file="$1"
  # 每一行应形如：数字 空格 数字 空格 规定字符串
  # 允许的状态：has taken a fork / is eating / is sleeping / is thinking / died
  bad="$(grep -nEv '^[0-9]+ [0-9]+ (has taken a fork|is eating|is sleeping|is thinking|died)$' "$file" || true)"
  if [ -n "$bad" ]; then
    printf "❌ 发现格式不对的行（行号:内容）：\n%s\n" "$bad"
    return 1
  fi
  printf "✅ 每行格式看起来都正确（时间戳 哲学家id 状态）\n"
  return 0
}

check_died_count() {
  file="$1"
  n="$(grep -c ' died$' "$file" || true)"
  printf "统计：died 行数 = %s\n" "$n"
  return 0
}

# ---------- 开始测试 ----------

print_head "Step 0: 先编译（make re）"
make re

if [ ! -x "$BIN" ]; then
  printf "❌ 找不到可执行文件：%s\n" "$BIN"
  printf "请确认 make 成功并生成 philo\n"
  exit 1
fi
printf "✅ 找到可执行文件：%s\n" "$BIN"

# Test 1：参数个数不对
# 期望：退出码非0，并输出 Error（你程序里可能是 Error: bad args）
run_case "t1_bad_argc" "$BIN 5 800 200"
printf "【说明】这个测试在测：参数数量不正确时是否会报错退出。\n"
printf "【期望】退出码非0；输出包含 Error 字样。\n"

# Test 2：参数不是纯数字
# 期望：退出码非0，并输出 Error
run_case "t2_not_number" "$BIN 5 800 abc 200"
printf "【说明】这个测试在测：出现非数字参数时是否会报错退出。\n"
printf "【期望】退出码非0；输出包含 Error 字样。\n"

# Test 3：只有1个哲学家（必死边界）
# 期望：最终会出现 '1 died'，并且程序会结束
run_case "t3_one_philo_die" "$BIN 1 300 100 100"
printf "【说明】这个测试在测：1个人只有1把叉，拿不到两把叉，应该会死亡。\n"
printf "【期望】输出里最后会出现类似：'<时间> 1 died'；程序能自己结束。\n"
check_died_count "$OUT_DIR/t3_one_philo_die.log"

# Test 4：有 must_eat（应在吃够后结束）
# 期望：通常不会出现 died，并且程序会结束（退出码0）
# 注意：并发有随机性，但这个参数较宽松，一般能顺利结束
run_case "t4_must_eat_finish" "$BIN 3 2000 200 200 2"
printf "【说明】这个测试在测：提供 must_eat 后，所有人吃够次数是否会自动结束。\n"
printf "【期望】退出码=0；通常没有 died（died=0）。\n"
check_died_count "$OUT_DIR/t4_must_eat_finish.log"

# Test 5：检查输出格式是否规整（不应出现乱码/半行）
# 期望：每行都符合：'数字 数字 状态' 的格式
print_head "t5_check_format (对 t4 的日志做格式检查)"
check_lines_format "$OUT_DIR/t4_must_eat_finish.log"
printf "【说明】这个测试在测：多线程打印是否被 print_lock 保护，行不会被切碎。\n"
printf "【期望】全部行都匹配规定格式。\n"

print_head "All done"
printf "日志都在：%s\n" "$OUT_DIR"
printf "你可以用：less -R %s/t4_must_eat_finish.log 查看完整输出\n" "$OUT_DIR"
