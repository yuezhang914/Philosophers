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
