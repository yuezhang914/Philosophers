# Philosophers

## Overview

**Philosophers** is a mandatory project from 42 School, based on the classic *Dining Philosophers Problem*.

The goal is to implement a correct and stable multi-threaded simulation using **POSIX threads (pthread)** and **mutexes**, while strictly avoiding data races, deadlocks, and undefined behavior.

### Key Concepts:

* **Threads:** Each philosopher is represented by a thread.
* **Mutexes:** Each fork is protected by a mutex.
* **Routine:** Philosophers repeatedly: **Eat  Sleep  Think**.
* **Death:** A philosopher dies if they do not start eating within `time_to_die`.
* **Completion:** If `must_eat` is provided, the simulation stops once all philosophers have eaten enough times.

---

## Build

To compile the project, run:

```bash
make

```

To rebuild from scratch:

```bash
make re

```

This will generate the executable: `./philo`.

---

## Usage

```bash
./philo number_of_philosophers time_to_die time_to_eat time_to_sleep [must_eat]

```

### Parameters

| Parameter | Description | Unit |
| --- | --- | --- |
| `number_of_philosophers` | Number of philosophers (and forks) | count |
| `time_to_die` | Time limit before a philosopher dies without eating | ms |
| `time_to_eat` | Duration of the eating state | ms |
| `time_to_sleep` | Duration of the sleeping state | ms |
| `must_eat` (optional) | Minimum meals per philosopher to end simulation | count |

---

## Output Format

Each log line strictly follows the format:

```text
<timestamp> <philosopher_id> <message>

```

*Example:*

```text
200 3 is eating
400 3 is sleeping

```

* **timestamp**: milliseconds since the start of the simulation.
* **philosopher_id**: index starting from 1.
* **Possible messages**: `has taken a fork`, `is eating`, `is sleeping`, `is thinking`, `died`.

---

## Design Overview

### Thread Model

* **Philosopher Threads:** One per philosopher.
* **Monitoring Thread:** A dedicated thread that:
* Detects philosopher death.
* Detects when all philosophers have eaten enough times.



### Mutex Strategy

* **Fork Mutexes:** One per fork.
* **Philosopher Mutexes:** One per philosopher to protect `last_meal_time` and `meals_eaten`.
* **Global Mutexes:**
* *Print Mutex*: Avoids scrambled output.
* *State Mutex*: Protects the global stop flag.



### Deadlock and Starvation Prevention

* **Pick-up Order:** Philosophers use different fork-picking orders based on their index (odd/even).
* **Desynchronization:** A dynamic thinking delay is introduced to desynchronize fork acquisition and reduce starvation risk.

### Time Management

* Millisecond precision using `gettimeofday`.
* **Custom Sleep:** An interruptible sleep function that periodically checks the global stop flag to ensure timely death detection (within 10ms as required).

---

## Project Status

* ✅ No data races (TSan verified)
* ✅ No memory leaks (Valgrind verified)
* ✅ Thread-safe logging
* ✅ Fully compliant with 42 evaluation requirements

---

---

# 哲学家进餐 (Philosophers)

## 项目简介

**Philosophers** 是 42 学校的 Mandatory 项目之一，基于经典的并发问题 *Dining Philosophers Problem*。

本项目要求使用 **POSIX 线程 (pthread)** 与 **互斥锁 (mutex)**，在严格避免数据竞争、死锁和未定义行为的前提下，实现一个稳定的并发模拟程序。

### 核心逻辑：

* **线程模型**：每个哲学家对应一个线程。
* **资源保护**：每把叉子由一个互斥锁保护。
* **行为循环**：哲学家循环执行：**吃  睡  想**。
* **死亡判定**：若超过 `time_to_die` 未开始进食，则哲学家死亡，模拟结束。
* **停止条件**：若指定 `must_eat`，当所有哲学家吃够次数后，模拟自动结束。

---

## 编译方式

编译项目：

```bash
make

```

重新编译：

```bash
make re

```

生成可执行文件：`./philo`。

---

## 使用方式

```bash
./philo 哲学家数量 存活时间 吃饭时间 睡觉时间 [最少吃饭次数]

```

### 参数说明

| 参数 | 含义 | 单位 |
| --- | --- | --- |
| `哲学家数量` | 哲学家及叉子的总数 | 个 |
| `存活时间` | 多久未进食会死亡 | 毫秒 |
| `吃饭时间` | 吃饭动作持续的时间 | 毫秒 |
| `睡觉时间` | 睡觉动作持续的时间 | 毫秒 |
| `最少吃饭次数` (可选) | 每个哲学家必须达到的进食次数 | 次 |

---

## 输出格式

程序输出严格遵循以下格式：

```text
<时间戳> <哲学家编号> <状态信息>

```

*示例：*

```text
200 3 is eating
400 3 is sleeping

```

* **时间戳**：从程序启动开始计算的毫秒数。
* **哲学家编号**：从 1 开始编号。
* **可能的状态**：`has taken a fork`, `is eating`, `is sleeping`, `is thinking`, `died`。

---

## 设计思路概览

### 线程模型

* **哲学家线程**：每位哲学家一个独立线程。
* **监控线程**：额外创建一个独立线程用于：
* 实时检测哲学家是否死亡。
* 检测是否所有哲学家已满足进食次数。



### 互斥锁设计

* **叉子锁**：每把叉子对应一个 `mutex`。
* **哲学家锁**：每个哲学家拥有独立的锁，用于保护 `last_meal_time` 和已进食次数。
* **全局锁**：
* *打印锁*：防止多线程同时输出导致字符错乱。
* *状态锁*：保护全局停止标志位（Stop Flag）。



### 死锁与饥饿避免

* **拿叉顺序**：根据哲学家编号的奇偶性，采用不同的拿叉顺序。
* **动态思考**：在思考阶段引入微小的动态延迟，使线程错峰执行，降低资源竞争。

### 时间与精度控制

* 使用 `gettimeofday` 获取毫秒级时间。
* **精准休眠**：实现可中断的睡眠机制，在休眠过程中持续检测停止条件，确保死亡检测精度在 10ms 以内。

---

## 项目状态

* ✅ 无数据竞争 (Data Race)
* ✅ 无内存泄漏 (Valgrind 认证)
* ✅ 日志输出线程安全
* ✅ 完整覆盖 Mandatory 要求，符合 42 评估标准

---
