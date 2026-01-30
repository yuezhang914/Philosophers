/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:10:44 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/30 15:27:46 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 获取当前时间（毫秒） */
long	time_ms(void)
{
	struct timeval	tv;

	if (gettimeofday(&tv, NULL) != 0)
		return (0);
	return (tv.tv_sec * 1000L + tv.tv_usec / 1000L);
}

/* 可被停止信号打断的睡眠：模拟结束时尽快醒来退出 */
void	wait_until_stop(t_sim *sim, long ms)
{
	long	start;

	start = time_ms();
	while (time_ms() - start < ms)
	{
		if (stop_get(sim))
			break ;
		usleep(250);
	}
}

/*
 * 计算 thinking 应该等待多久（毫秒）。
 * 目的：让大家不要“同一时刻一起抢叉”，减少饥饿概率。
 */
long	think_ms(t_sim *sim)
{
	long	spare;
	long	wait;

	spare = (long)sim->die_ms - (long)sim->eat_ms - (long)sim->sleep_ms;
	wait = spare / 2;
	if (wait < 0)
		return (0);
	if ((sim->count % 2) == 1 && wait > 0)
		wait += 1;
	return (wait);
}
