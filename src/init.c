/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:10:05 by yzhang2           #+#    #+#             */
/*   Updated: 2025/12/16 00:10:22 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 初始化模拟需要的锁：打印锁、状态锁、每把叉子的锁 */
int	sim_init_mutex(t_sim *sim)
{
	int	i;

	if (pthread_mutex_init(&sim->print_lock, NULL) != 0)
		return (1);
	sim->print_inited = 1;
	if (pthread_mutex_init(&sim->state_lock, NULL) != 0)
		return (1);
	sim->state_inited = 1;
	i = 0;
	while (i < sim->count)
	{
		if (pthread_mutex_init(&sim->forks[i], NULL) != 0)
			return (1);
		sim->fork_inited += 1;
		i++;
	}
	return (0);
}

/* 初始化每个哲学家的数据：编号、左右叉子、吃饭计数、上次吃饭时间 */
int	sim_init_philo(t_sim *sim, t_philo *ph)
{
	int	i;

	sim->start_ms = time_ms();
	i = 0;
	while (i < sim->count)
	{
		ph[i].id = i + 1;
		ph[i].meals = 0;
		ph[i].last_meal = sim->start_ms;
		ph[i].left = &sim->forks[i];
		ph[i].right = &sim->forks[(i + 1) % sim->count];
		ph[i].sim = sim;
		if (pthread_mutex_init(&ph[i].meal_lock, NULL) != 0)
			return (1);
		sim->meal_inited += 1;
		i++;
	}
	return (0);
}
