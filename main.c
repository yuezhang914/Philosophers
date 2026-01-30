/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:00:00 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/30 16:24:38 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 分配内存 + 初始化锁 + 初始化每个哲学家的数据 */
static int	sim_build(t_sim *sim, t_philo **ph, pthread_t **th)
{
	sim->forks = malloc(sizeof(*sim->forks) * sim->count);
	*ph = malloc(sizeof(**ph) * sim->count);
	*th = malloc(sizeof(**th) * sim->count);
	if (!sim->forks || !*ph || !*th)
	{
		sim_release(sim, *ph, *th);
		return (print_err("malloc failed"));
	}
	if (sim_init_mutex(sim) != 0 || sim_init_philo(sim, *ph) != 0)
	{
		sim_release(sim, *ph, *th);
		return (print_err("init failed"));
	}
	return (0);
}

/* 启动哲学家线程 + 启动监控线程 */
static int	sim_start(t_sim *sim, t_philo *ph, pthread_t *th, pthread_t *watch)
{
	if (start_philos(sim, ph, th) != 0)
		return (1);
	if (pthread_create(watch, NULL, watch_thread, ph) != 0)
	{
		stop_set(sim);
		join_philos(th, sim->count);
		return (print_err("watch thread failed"));
	}
	return (0);
}

/* 等待线程结束 + 清理所有资源 */
static void	sim_finish(t_sim *sim, t_philo *ph, pthread_t *th, pthread_t watch)
{
	pthread_join(watch, NULL);
	join_philos(th, sim->count);
	sim_release(sim, ph, th);
}

/* 程序入口：按顺序做参数解析、搭建模拟、运行、收尾 */
int	main(int argc, char **argv)
{
	t_sim		sim;
	t_philo		*ph;
	pthread_t	*th;
	pthread_t	watch;

	ph = NULL;
	th = NULL;
	if (sim_parse(argc, argv, &sim) != 0)
		return (print_err("bad args"));
	if (sim_build(&sim, &ph, &th) != 0)
		return (1);
	if (sim_start(&sim, ph, th, &watch) != 0)
	{
		sim_release(&sim, ph, th);
		return (1);
	}
	sim_finish(&sim, ph, th, watch);
	return (0);
}
