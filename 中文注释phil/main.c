/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:00:00 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:36:56 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * sim_build
 * 作用：
 * 1) 为 forks / philo / thread 分配内存
 * 2) 初始化各种 mutex
 * 3) 初始化每个哲学家的初始数据（左右叉子指针、last_meal 等）
 *
 * 参数：
 * - sim：全局模拟器（里面会保存 forks 指针、start_ms 等）
 * - ph：输出参数，返回“哲学家数组”的指针
 * - th：输出参数，返回“线程数组”的指针
 *
 * 返回值：
 * - 0：成功
 * - 1：失败（并且会释放掉已经分配/初始化的资源）
 * ========================= */
static int	sim_build(t_sim *sim, t_philo **ph, pthread_t **th)
{
	/* 分配 forks：每个哲学家左右各一把叉子，但叉子总数=哲学家数 */
	sim->forks = malloc(sizeof(*sim->forks) * sim->count);

	/* 分配哲学家数组：t_philo[sim->count] */
	*ph = malloc(sizeof(**ph) * sim->count);

	/* 分配线程数组：pthread_t[sim->count] */
	*th = malloc(sizeof(**th) * sim->count);

	/* 任何一个 malloc 失败都要清理（free + destroy） */
	if (!sim->forks || !*ph || !*th)
	{
		sim_release(sim, *ph, *th);
		return (print_err("malloc failed"));
	}

	/* 初始化锁 + 初始化哲学家数据
	 * 任何一步失败也要清理，避免资源泄漏
	 */
	if (sim_init_mutex(sim) != 0 || sim_init_philo(sim, *ph) != 0)
	{
		sim_release(sim, *ph, *th);
		return (print_err("init failed"));
	}
	return (0);
}

/* =========================
 * sim_start
 * 作用：
 * 1) 创建所有哲学家线程
 * 2) 再创建监控线程（watch）
 *
 * 参数：
 * - sim：全局模拟器
 * - ph：哲学家数组
 * - th：哲学家线程句柄数组
 * - watch：输出参数，返回监控线程句柄
 *
 * 返回值：
 * - 0：成功
 * - 1：失败
 * ========================= */
static int	sim_start(t_sim *sim, t_philo *ph, pthread_t *th, pthread_t *watch)
{
	/* 先启动哲学家线程 */
	if (start_philos(sim, ph, th) != 0)
		return (1);

	/* 再启动监控线程：把 ph 作为参数传进去（watch 要遍历 ph 数组） */
	if (pthread_create(watch, NULL, watch_thread, ph) != 0)
	{
		/* 监控线程起不来，就让模拟停止，并回收已经启动的哲学家线程 */
		stop_set(sim);
		join_philos(th, sim->count);
		return (print_err("watch thread failed"));
	}
	return (0);
}

/* =========================
 * sim_finish
 * 作用：
 * 1) 等待监控线程结束
 * 2) 等待所有哲学家线程结束
 * 3) 释放资源
 *
 * 参数：
 * - sim：全局模拟器
 * - ph：哲学家数组
 * - th：线程数组
 * - watch：监控线程句柄
 * ========================= */
static void	sim_finish(t_sim *sim, t_philo *ph, pthread_t *th, pthread_t watch)
{
	/* 监控线程一定会在“有人死/大家吃够/外部 stop”时退出 */
	pthread_join(watch, NULL);

	/* 让所有哲学家线程也结束并回收 */
	join_philos(th, sim->count);

	/* 销毁 mutex + free 内存 */
	sim_release(sim, ph, th);
}

/* =========================
 * main
 * 程序入口：
 * 1) parse 参数
 * 2) build（分配 + 初始化）
 * 3) start（启动线程）
 * 4) finish（join + 清理）
 * ========================= */
int	main(int argc, char **argv)
{
	t_sim		sim;    /* 全局模拟器：配置 + 共享状态 */
	t_philo		*ph;    /* 哲学家数组（动态分配） */
	pthread_t	*th;    /* 哲学家线程数组（动态分配） */
	pthread_t	watch;  /* 监控线程句柄 */

	/* 防止 release 时 free 野指针：先置 NULL */
	ph = NULL;
	th = NULL;

	/* 解析参数：失败就直接退出 */
	if (sim_parse(argc, argv, &sim) != 0)
		return (print_err("bad args"));

	/* 分配资源并初始化：失败就退出 */
	if (sim_build(&sim, &ph, &th) != 0)
		return (1);

	/* 启动线程：失败也要释放资源 */
	if (sim_start(&sim, ph, th, &watch) != 0)
	{
		sim_release(&sim, ph, th);
		return (1);
	}

	/* 正常收尾：join + destroy + free */
	sim_finish(&sim, ph, th, watch);
	return (0);
}
