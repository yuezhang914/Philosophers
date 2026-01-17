/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   state.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:11:06 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:37:45 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * stop_get
 * 作用：安全读取 sim->stop
 * 为什么要锁：
 * - 多线程下，一个线程写 stop，另一个线程读 stop
 * - 没锁可能读到“撕裂/旧值”（并发问题）
 *
 * 参数：sim - 全局模拟器
 * 返回：stop 的值（0 或 1）
 * ========================= */
int	stop_get(t_sim *sim)
{
	int	v;

	pthread_mutex_lock(&sim->state_lock);
	v = sim->stop;
	pthread_mutex_unlock(&sim->state_lock);
	return (v);
}

/* =========================
 * stop_set
 * 作用：安全设置 sim->stop = 1
 * 参数：sim - 全局模拟器
 * ========================= */
void	stop_set(t_sim *sim)
{
	pthread_mutex_lock(&sim->state_lock);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->state_lock);
}

/* =========================
 * log_msg
 * 作用：打印一条状态日志（timestamp id msg）
 *
 * 关键点：
 * 1) 先用 state_lock 检查 stop
 *    - 如果 stop=1，并且 force=0，就直接不打印（避免结束后还刷日志）
 * 2) 再用 print_lock 包住 printf，保证多线程输出不交叉
 *
 * 参数：
 * - sim：全局模拟器（里面有锁、start_ms、stop）
 * - id：哲学家编号
 * - msg：状态字符串（"is eating" 等）
 * - force：是否强制打印（死亡日志 died 需要强制打印）
 * ========================= */
void	log_msg(t_sim *sim, int id, const char *msg, int force)
{
	long	ts;

	/* 用 state_lock 保护 stop 检查与后续打印判断 */
	pthread_mutex_lock(&sim->state_lock);

	/* 如果不是强制打印，并且已经 stop，就不再输出 */
	if (!force && sim->stop)
	{
		pthread_mutex_unlock(&sim->state_lock);
		return ;
	}

	/* 拿到 print_lock 后再 printf，避免输出打架 */
	pthread_mutex_lock(&sim->print_lock);

	/* 计算相对时间戳：当前毫秒 - start_ms */
	ts = time_ms() - sim->start_ms;

	printf("%ld %d %s\n", ts, id, msg);

	pthread_mutex_unlock(&sim->print_lock);
	pthread_mutex_unlock(&sim->state_lock);
}
