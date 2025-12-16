/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   watch.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:11:58 by yzhang2           #+#    #+#             */
/*   Updated: 2025/12/16 01:07:35 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 安全读取一个哲学家的 last_meal（用 meal_lock 保护） */
static long	read_last_meal(t_philo *p)
{
	long	t;

	pthread_mutex_lock(&p->meal_lock);
	t = p->last_meal;
	pthread_mutex_unlock(&p->meal_lock);
	return (t);
}

/* 判断某个哲学家是否已经超过 die_ms 没吃饭了 */
static int	is_dead(t_sim *sim, t_philo *p)
{
	long	last;

	last = read_last_meal(p);
	if (time_ms() - last >= sim->die_ms)
		return (1);
	return (0);
}

/* 判断是否所有人都吃够了 must_eat 次（没有 must_eat 就返回 0） */
static int	all_full(t_sim *sim, t_philo *ph)
{
	int	i;
	int	ok;

	if (sim->must_eat <= 0)
		return (0);
	i = 0;
	while (i < sim->count)
	{
		pthread_mutex_lock(&ph[i].meal_lock);
		ok = (ph[i].meals >= sim->must_eat);
		pthread_mutex_unlock(&ph[i].meal_lock);
		if (!ok)
			return (0);
		i++;
	}
	return (1);
}

/* 扫描是否有人死亡：发现死亡就设置 stop 并打印 died */
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

/* 监控线程：循环检查死亡和吃够次数，控制模拟结束 */
void	*watch_thread(void *arg)
{
	t_philo	*ph;
	t_sim	*sim;

	ph = (t_philo *)arg;
	sim = ph[0].sim;
	while (!stop_get(sim))
	{
		if (scan_dead(sim, ph))
			return (NULL);
		if (all_full(sim, ph))
		{
			stop_set(sim);
			return (NULL);
		}
		usleep(1000);
	}
	return (NULL);
}
