/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   clean.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:13:13 by yzhang2           #+#    #+#             */
/*   Updated: 2025/12/16 01:07:11 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 打印错误信息到标准错误输出 */
int	print_err(const char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	return (1);
}

/* 销毁每个哲学家的 meal_lock（只销毁已初始化的那部分） */
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

/* 销毁每把叉子的锁（只销毁已初始化的那部分） */
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

/* 释放所有资源：锁、数组、指针（保证不会 destroy 未初始化的锁） */
void	sim_release(t_sim *sim, t_philo *ph, pthread_t *th)
{
	destroy_meal_lock(sim, ph);
	destroy_fork_lock(sim);
	if (sim->print_inited)
		pthread_mutex_destroy(&sim->print_lock);
	if (sim->state_inited)
		pthread_mutex_destroy(&sim->state_lock);
	if (th)
		free(th);
	if (sim->forks)
		free(sim->forks);
	if (ph)
		free(ph);
}
