/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   watch.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:11:58 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:38:17 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * read_last_meal
 * 作用：安全读取某个哲学家的 last_meal
 * 为什么要锁：
 * - 哲学家线程在吃饭时会写 last_meal
 * - 监控线程在巡逻时会读 last_meal
 * - 不加锁就是数据竞争
 *
 * 参数：p - 哲学家
 * 返回：last_meal 的值（ms）
 * ========================= */
static long	read_last_meal(t_philo *p)
{
	long	t;

	pthread_mutex_lock(&p->meal_lock);
	t = p->last_meal;
	pthread_mutex_unlock(&p->meal_lock);
	return (t);
}

/* =========================
 * is_dead
 * 作用：判断某个哲学家是否“饿死”
 * 判断条件：
 * - 当前时间 - last_meal >= die_ms
 *
 * 参数：
 * - sim：全局模拟器（提供 die_ms）
 * - p：哲学家
 *
 * 返回：1 表示已死亡；0 表示没死
 * ========================= */
static int	is_dead(t_sim *sim, t_philo *p)
{
	long	last;

	last = read_last_meal(p);
	if (time_ms() - last >= sim->die_ms)
		return (1);
	return (0);
}

/* =========================
 * all_full
 * 作用：判断是否“所有人都吃够 must_eat 次”
 *
 * 关键点：
 * - 如果 must_eat <= 0，表示没有这个限制（直接返回 0）
 * - 否则检查每个 ph[i].meals >= must_eat
 * - 读 meals 时必须加 meal_lock
 *
 * 参数：
 * - sim：全局模拟器（must_eat / count）
 * - ph：哲学家数组
 *
 * 返回：
 * - 1：所有人吃够了
 * - 0：有人没吃够，或者根本没启用 must_eat
 * ========================= */
static int	all_full(t_sim *sim, t_philo *ph)
{
	int	i;
	int	ok;

	/* must_eat 没设置或无意义，就不做“吃够判断” */
	if (sim->must_eat <= 0)
		return (0);

	i = 0;
	while (i < sim->count)
	{
		pthread_mutex_lock(&ph[i].meal_lock);
		ok = (ph[i].meals >= sim->must_eat);
		pthread_mutex_unlock(&ph[i].meal_lock);

		/* 只要有一个人没吃够，就不是 all_full */
		if (!ok)
			return (0);
		i++;
	}
	return (1);
}

/* =========================
 * scan_dead
 * 作用：扫描是否有人死亡
 * - 逐个检查 is_dead
 * - 一旦发现死亡：
 *   1) stop_set(sim)
 *   2) 强制打印 died（force=1，避免 stop 后不让打印）
 *   3) 返回 1
 *
 * 参数：
 * - sim：全局模拟器
 * - ph：哲学家数组
 *
 * 返回：
 * - 1：发现死亡
 * - 0：没人死
 * ========================= */
static int	scan_dead(t_sim *sim, t_philo *ph)
{
	int	i;

	i = 0;
	while (i < sim->count && !stop_get(sim))
	{
		if (is_dead(sim, &ph[i]))
		{
			stop_set(sim);
			log_msg(sim, ph[i].id, "died", 1);
			return (1);
		}
		i++;
	}
	return (0);
}

/* =========================
 * watch_thread
 * 作用：监控线程主循环
 * - 周期性检查：
 *   1) 是否有人死了
 *   2) 是否所有人都吃够了
 * - 如果满足任何一个结束条件，就 stop 并退出
 *
 * 参数：
 * - arg：传进来的哲学家数组首地址（t_philo*）
 *
 * 返回：NULL
 * ========================= */
void	*watch_thread(void *arg)
{
	t_philo	*ph;
	t_sim	*sim;

	ph = (t_philo *)arg;
	sim = ph[0].sim;

	while (!stop_get(sim))
	{
		/* 先判死：一旦有人死，立刻结束 */
		if (scan_dead(sim, ph))
			return (NULL);

		/* 再判全员吃够：如果 must_eat 启用且满足，就结束 */
		if (all_full(sim, ph))
		{
			stop_set(sim);
			return (NULL);
		}

		/* 监控线程也不要忙等，睡 1ms 左右 */
		usleep(1000);
	}
	return (NULL);
}
