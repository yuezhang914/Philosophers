/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:10:05 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:37:19 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * sim_init_mutex
 * 作用：初始化模拟需要的所有 mutex
 * - print_lock：保证日志打印不乱
 * - state_lock：保护 stop 标志
 * - forks[i]：每把叉子一个锁
 *
 * 参数：sim - 全局模拟器（forks 已经 malloc 过）
 * 返回：0 成功，1 失败
 * ========================= */
int	sim_init_mutex(t_sim *sim)
{
	int	i;

	/* 初始化打印锁：失败就直接返回 */
	if (pthread_mutex_init(&sim->print_lock, NULL) != 0)
		return (1);
	sim->print_inited = 1;

	/* 初始化状态锁：失败就返回（release 会根据 print_inited/state_inited 处理） */
	if (pthread_mutex_init(&sim->state_lock, NULL) != 0)
		return (1);
	sim->state_inited = 1;

	/* 初始化每一把叉子的锁 */
	i = 0;
	while (i < sim->count)
	{
		if (pthread_mutex_init(&sim->forks[i], NULL) != 0)
			return (1);
		/* 记录已经成功 init 了几把叉子锁 */
		sim->fork_inited += 1;
		i++;
	}
	return (0);
}

/* =========================
 * sim_init_philo
 * 作用：初始化每个哲学家的数据
 * - id：1..count
 * - meals：0
 * - last_meal：初始为 start_ms（表示“刚开始就算吃过一次的时间点”，避免立刻判死）
 * - left/right：指向 forks 数组里的两把叉子
 * - sim：回指全局模拟器
 * - meal_lock：保护 last_meal/meals
 *
 * 参数：
 * - sim：全局模拟器
 * - ph：哲学家数组（已经 malloc）
 *
 * 返回：0 成功，1 失败
 * ========================= */
int	sim_init_philo(t_sim *sim, t_philo *ph)
{
	int	i;

	/* 记录模拟开始时间：所有打印都用它做“相对时间” */
	sim->start_ms = time_ms();

	i = 0;
	while (i < sim->count)
	{
		/* 哲学家编号从 1 开始（更符合题目日志格式） */
		ph[i].id = i + 1;

		/* 初始吃饭次数为 0 */
		ph[i].meals = 0;

		/* 初始 last_meal = start_ms（表示“刚开始时刚吃过”） */
		ph[i].last_meal = sim->start_ms;

		/* 左叉子：第 i 把 */
		ph[i].left = &sim->forks[i];

		/* 右叉子：第 (i+1)%count 把（最后一个人的右叉子是第 0 把） */
		ph[i].right = &sim->forks[(i + 1) % sim->count];

		/* 保存全局指针：线程里要用配置、stop、锁等 */
		ph[i].sim = sim;

		/* 初始化这个哲学家的 meal_lock */
		if (pthread_mutex_init(&ph[i].meal_lock, NULL) != 0)
			return (1);

		/* 记录成功初始化了几个 meal_lock，方便失败时释放 */
		sim->meal_inited += 1;

		i++;
	}
	return (0);
}
