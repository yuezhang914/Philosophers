/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:10:44 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/30 11:24:57 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * time_ms
 * 作用：获取当前时间（毫秒）
 * 实现：gettimeofday 得到秒+微秒，再换算成毫秒
 *
 * 返回：
 * - >=0：当前时间戳（ms）
 * - 0：gettimeofday 失败时返回 0（这里简单处理）
 * ========================= */
long	time_ms(void)
{
	struct timeval	tv;

	if (gettimeofday(&tv, NULL) != 0)
		return (0);
	return (tv.tv_sec * 1000L + tv.tv_usec / 1000L);
}

/* =========================
 * wait_until_stop
 * 作用：可以被 stop 信号打断的睡眠
 * 为什么需要：
 * - 如果模拟已经结束（stop=1），线程不该傻等睡满 sleep_ms/eat_ms
 * - 应该尽快醒来退出，join 才快
 *
 * 参数：
 * - sim：用来随时 stop_get
 * - ms：要睡的毫秒数
 * ========================= */
void	wait_until_stop(t_sim *sim, long ms)
{
	long	start;

	start = time_ms();
	while (time_ms() - start < ms)
	{
		/* 如果已经 stop，就立刻提前结束睡眠 */
		if (stop_get(sim))
			break ;
		usleep(250);
	}
}
