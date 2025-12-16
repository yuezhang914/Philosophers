/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   state.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:11:06 by yzhang2           #+#    #+#             */
/*   Updated: 2025/12/16 01:07:05 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 读取 stop 标志：判断模拟是否要停止 */
int	stop_get(t_sim *sim)
{
	int	v;

	pthread_mutex_lock(&sim->state_lock);
	v = sim->stop;
	pthread_mutex_unlock(&sim->state_lock);
	return (v);
}

/* 设置 stop 标志：告诉所有线程该结束了 */
void	stop_set(t_sim *sim)
{
	pthread_mutex_lock(&sim->state_lock);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->state_lock);
}

/* 打印一条状态日志：用锁保证输出不乱 */
void	log_msg(t_sim *sim, int id, const char *msg, int force)
{
	long	ts;

	pthread_mutex_lock(&sim->state_lock);
	if (!force && sim->stop)
	{
		pthread_mutex_unlock(&sim->state_lock);
		return ;
	}
	pthread_mutex_lock(&sim->print_lock);
	ts = time_ms() - sim->start_ms;
	printf("%ld %d %s\n", ts, id, msg);
	pthread_mutex_unlock(&sim->print_lock);
	pthread_mutex_unlock(&sim->state_lock);
}
