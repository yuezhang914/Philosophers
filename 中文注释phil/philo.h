/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   philo.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:00:00 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/30 11:24:57 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PHILO_H
# define PHILO_H

/* limits.h
 * - 用到了：INT_MAX / LONG_MAX
 *   parse.c 里做“范围检查”和“溢出判断”需要它们。
 */
# include <limits.h>

/* pthread.h
 * - 用到了：pthread_t / pthread_create / pthread_join
 * - 用到了：pthread_mutex_t / pthread_mutex_init/destroy/lock/unlock
 *   这是多线程和互斥锁的核心库。
 */
# include <pthread.h>

/* stdio.h
 * - 用到了：printf / fprintf
 *   打印状态日志、打印错误信息。
 */
# include <stdio.h>

/* stdlib.h
 * - 用到了：malloc / free
 *   分配 forks/philo/thread 数组并释放。
 */
# include <stdlib.h>

/* sys/time.h
 * - 用到了：gettimeofday
 *   用来获得当前时间（毫秒）。
 */
# include <sys/time.h>

/* unistd.h
 * - 用到了：usleep
 *   用“短睡眠 + 循环检查”实现毫秒级 sleep。
 */
# include <unistd.h>

/* =========================
 * t_sim：整个模拟（全局共享状态）
 * ========================= */
typedef struct s_sim
{
	/* 哲学家数量（也等于叉子数量） */
	int				count;

	/* 超过多少毫秒不吃饭就会死 */
	int				die_ms;

	/* 每次吃饭要持续多少毫秒 */
	int				eat_ms;

	/* 每次睡觉要持续多少毫秒 */
	int				sleep_ms;

	/* 每个人必须吃多少次（可选参数）
	 * - 如果没给这个参数，代码里设置为 -1 表示“不要求吃够次数”
	 */
	int				must_eat;

	/* stop：全局停止信号（0=继续，1=停止）
	 * - 任何线程都可能读它
	 * - 监控线程可能写它
	 * - 所以读写都要用 state_lock 保护
	 */
	int				stop;

	/* start_ms：模拟开始的时间戳（毫秒）
	 * 打印日志时会输出：当前时间 - start_ms
	 */
	long			start_ms;

	/* 下面这几个 *_inited 是“已经初始化了多少个锁”的计数器：
	 * 作用：如果中途初始化失败，释放时只 destroy 已经 init 成功的锁，
	 * 避免 destroy 未初始化锁导致未定义行为。
	 */
	int				fork_inited;   /* forks 数组里已 init 的 mutex 个数 */
	int				meal_inited;   /* philo 数组里已 init 的 meal_lock 个数 */
	int				print_inited;  /* print_lock 是否 init 成功（0/1） */
	int				state_inited;  /* state_lock 是否 init 成功（0/1） */

	/* forks：叉子锁数组
	 * forks[i] 表示第 i 把叉子的 mutex
	 */
	pthread_mutex_t	*forks;

	/* print_lock：保护 printf 输出，避免多线程输出交叉 */
	pthread_mutex_t	print_lock;

	/* state_lock：保护 stop 标志（以及 log_msg 里 stop 检查的原子性） */
	pthread_mutex_t	state_lock;
}					t_sim;

/* =========================
 * t_philo：单个哲学家（每个线程一个）
 * ========================= */
typedef struct s_philo
{
	/* 哲学家编号：从 1 开始（打印时用） */
	int				id;

	/* 这个哲学家已经吃了多少次（用于 must_eat 判断） */
	int				meals;

	/* last_meal：最后一次开始吃饭的时间戳（毫秒）
	 * 监控线程会用它判断“多久没吃 -> 是否死亡”
	 */
	long			last_meal;

	/* meal_lock：保护 meals 和 last_meal
	 * 因为：
	 * - 哲学家线程在吃饭时会写 last_meal/meals
	 * - 监控线程会读 last_meal/meals
	 * 所以必须加锁避免数据竞争。
	 */
	pthread_mutex_t	meal_lock;

	/* left/right：指向“叉子锁”的指针
	 * 不是复制 mutex，而是指向 sim->forks[] 里面的某两个 mutex
	 */
	pthread_mutex_t	*left;
	pthread_mutex_t	*right;

	/* sim：指向全局模拟器，方便线程访问共享配置和 stop 标志 */
	t_sim			*sim;
}					t_philo;

/* ===== 参数解析 ===== */
int					sim_parse(int argc, char **argv, t_sim *sim);

/* ===== 初始化（锁 + 哲学家信息） ===== */
int					sim_init_mutex(t_sim *sim);
int					sim_init_philo(t_sim *sim, t_philo *ph);

/* ===== 时间工具 ===== */
long				time_ms(void);
void				wait_until_stop(t_sim *sim, long ms);

/* ===== 共享状态（stop + 打印） ===== */
int					stop_get(t_sim *sim);
void				stop_set(t_sim *sim);
void				log_msg(t_sim *sim, int id, const char *msg, int force);

/* ===== 哲学家线程启动/回收 ===== */
void				*philo_thread(void *arg);
int					start_philos(t_sim *sim, t_philo *ph, pthread_t *th);
void				join_philos(pthread_t *th, int n);

/* ===== 监控线程 ===== */
void				*watch_thread(void *arg);

/* ===== 错误输出 + 清理 ===== */
int					print_err(const char *msg);
void				sim_release(t_sim *sim, t_philo *ph, pthread_t *th);

#endif
