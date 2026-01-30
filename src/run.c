/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:11:25 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/30 15:40:45 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 决定拿叉顺序：让部分人先拿右叉，减少死锁风险 */
static void	pick_order(t_philo *p, pthread_mutex_t **first,
		pthread_mutex_t **sec)
{
	if ((p->id % 2) == 0)
	{
		*first = p->right;
		*sec = p->left;
	}
	else
	{
		*first = p->left;
		*sec = p->right;
	}
}

/* 做一次吃饭：拿两把叉、更新吃饭时间、睡 eat_ms、再放下叉子 */
static void	eat_once(t_philo *p)
{
	pthread_mutex_t	*first;
	pthread_mutex_t	*sec;
	t_sim			*sim;

	sim = p->sim;
	pick_order(p, &first, &sec);
	pthread_mutex_lock(first);
	log_msg(sim, p->id, "has taken a fork", 0);
	if (sim->count == 1)
	{
		wait_until_stop(sim, sim->die_ms);
		pthread_mutex_unlock(first);
		return ;
	}
	pthread_mutex_lock(sec);
	log_msg(sim, p->id, "has taken a fork", 0);
	pthread_mutex_lock(&p->meal_lock);
	p->last_meal = time_ms();
	p->meals += 1;
	pthread_mutex_unlock(&p->meal_lock);
	log_msg(sim, p->id, "is eating", 0);
	wait_until_stop(sim, sim->eat_ms);
	pthread_mutex_unlock(sec);
	pthread_mutex_unlock(first);
}
/* 哲学家线程：不断吃、睡、想，直到 stop */
void	*philo_thread(void *arg)
{
	t_philo	*p;
	t_sim	*sim;

	p = (t_philo *)arg;
	sim = p->sim;
	if ((p->id % 2) == 0)
		usleep(500);
	while (!stop_get(sim) && !philo_done(p))
	{
		eat_once(p);
		if (stop_get(sim) || philo_done(p))
			break ;
		log_msg(sim, p->id, "is sleeping", 0);
		wait_until_stop(sim, sim->sleep_ms);
		if (stop_get(sim) || philo_done(p))
			break ;
		log_msg(sim, p->id, "is thinking", 0);
		wait_until_stop(sim, think_ms(sim));
	}
	return (NULL);
}

/* 创建所有哲学家线程，失败时要立刻停掉并回收已创建线程 */
int	start_philos(t_sim *sim, t_philo *ph, pthread_t *th)
{
	int	i;

	i = 0;
	while (i < sim->count)
	{
		if (pthread_create(&th[i], NULL, philo_thread, &ph[i]) != 0)
		{
			stop_set(sim);
			join_philos(th, i);
			return (print_err("philo thread failed"));
		}
		i++;
	}
	return (0);
}

/* 等待所有哲学家线程结束 */
void	join_philos(pthread_t *th, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		pthread_join(th[i], NULL);
		i++;
	}
}
