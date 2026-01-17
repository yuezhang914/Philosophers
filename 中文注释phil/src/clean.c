/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   clean.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:13:13 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:37:09 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * print_err
 * 作用：把错误信息打印到标准错误输出 stderr
 * 参数：msg - 错误描述字符串
 * 返回：固定返回 1（方便 main 里直接 return）
 * ========================= */
int	print_err(const char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	return (1);
}

/* =========================
 * destroy_meal_lock
 * 作用：销毁每个哲学家的 meal_lock
 * 关键点：只销毁“已经成功初始化”的那部分
 *
 * 为什么要 sim->meal_inited：
 * - init 过程中可能第 k 个 mutex_init 失败
 * - 这时只有 [0..k-1] 是 init 成功的
 * - 只 destroy 这些，避免 destroy 未初始化锁导致未定义行为
 * ========================= */
static void	destroy_meal_lock(t_sim *sim, t_philo *ph)
{
	int	i;

	i = 0;
	while (ph && i < sim->meal_inited)
	{
		pthread_mutex_destroy(&ph[i].meal_lock);
		i++;
	}
}

/* =========================
 * destroy_fork_lock
 * 作用：销毁 forks 数组里的 mutex
 * 同理：只销毁已经 init 的那部分（sim->fork_inited）
 * ========================= */
static void	destroy_fork_lock(t_sim *sim)
{
	int	i;

	i = 0;
	while (sim->forks && i < sim->fork_inited)
	{
		pthread_mutex_destroy(&sim->forks[i]);
		i++;
	}
}

/* =========================
 * sim_release
 * 作用：统一释放资源（destroy mutex + free 内存）
 *
 * 参数：
 * - sim：全局模拟器（里面有 forks 指针、各种 *_inited 标记）
 * - ph：哲学家数组指针（可能为 NULL）
 * - th：线程数组指针（可能为 NULL）
 *
 * 设计思想：
 * - “即使中途失败也能安全释放”
 * - 所以：
 *   1) destroy 之前先看是否 init 过（*_inited）
 *   2) free 之前先判断指针是否为 NULL
 * ========================= */
void	sim_release(t_sim *sim, t_philo *ph, pthread_t *th)
{
	/* 先销毁哲学家自己的锁（依赖 ph） */
	destroy_meal_lock(sim, ph);

	/* 再销毁叉子锁（依赖 sim->forks） */
	destroy_fork_lock(sim);

	/* print_lock 只有 init 成功才 destroy */
	if (sim->print_inited)
		pthread_mutex_destroy(&sim->print_lock);

	/* state_lock 只有 init 成功才 destroy */
	if (sim->state_inited)
		pthread_mutex_destroy(&sim->state_lock);

	/* 最后释放动态分配的内存 */
	if (th)
		free(th);
	if (sim->forks)
		free(sim->forks);
	if (ph)
		free(ph);
}
