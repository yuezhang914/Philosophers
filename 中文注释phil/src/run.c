/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:11:25 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:38:06 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * pick_order
 * 作用：决定“拿叉顺序”
 *
 * 为什么要决定顺序：
 * - 如果所有人都“先拿左叉”，就容易出现：
 *   每个人都拿到左叉，然后都在等右叉 -> 死锁风险大
 * - 你这里用“奇偶策略”：
 *   偶数号先拿右叉，奇数号先拿左叉
 *   让拿叉方向不完全一致，降低死锁概率
 *
 * 参数：
 * - p：哲学家对象
 * - first：输出参数，返回第一把要拿的叉子 mutex 指针
 * - sec：输出参数，返回第二把要拿的叉子 mutex 指针
 * ========================= */
static void	pick_order(t_philo *p, pthread_mutex_t **first,
		pthread_mutex_t **sec)
{
	/* 偶数号：先右后左 */
	if ((p->id % 2) == 0)
	{
		*first = p->right;
		*sec = p->left;
	}
	/* 奇数号：先左后右 */
	else
	{
		*first = p->left;
		*sec = p->right;
	}
}

/* =========================
 * eat_once
 * 作用：完成“一次吃饭动作”
 * 流程：
 * 1) 决定拿叉顺序
 * 2) lock 第一把叉 -> 打印拿叉
 * 3) 若只有 1 个哲学家：永远拿不到第二把叉
 *    - 让他一直等到 die_ms（用 sleep_ms_stop）
 *    - 然后释放第一把叉并返回
 * 4) lock 第二把叉 -> 打印拿叉
 * 5) 更新 last_meal/meals（必须 meal_lock 保护）
 * 6) 打印 is eating
 * 7) 睡 eat_ms（可被 stop 打断）
 * 8) unlock 两把叉
 *
 * 参数：p - 当前哲学家
 * ========================= */
static void	eat_once(t_philo *p)
{
	pthread_mutex_t	*first; /* 第一把叉 */
	pthread_mutex_t	*sec;   /* 第二把叉 */
	t_sim			*sim;   /* 全局模拟器指针（方便用） */

	sim = p->sim;

	/* 根据奇偶决定先拿哪把叉 */
	pick_order(p, &first, &sec);

	/* 拿第一把叉 */
	pthread_mutex_lock(first);
	log_msg(sim, p->id, "has taken a fork", 0);

	/* 特判：只有一个哲学家时，左右叉其实是同一把或无法形成“两把”
	 * 结果：他永远拿不到第二把叉，会饿死
	 * 做法：拿着这一把叉，等到 die_ms 后退出（watch 会判死）
	 */
	if (sim->count == 1)
	{
		sleep_ms_stop(sim, sim->die_ms);
		pthread_mutex_unlock(first);
		return ;
	}

	/* 拿第二把叉 */
	pthread_mutex_lock(sec);
	log_msg(sim, p->id, "has taken a fork", 0);

	/* 更新 last_meal / meals 必须加 meal_lock
	 * 因为 watch_thread 也会读这些数据
	 */
	pthread_mutex_lock(&p->meal_lock);
	p->last_meal = time_ms();
	p->meals += 1;
	pthread_mutex_unlock(&p->meal_lock);

	/* 打印并睡一段时间表示正在吃 */
	log_msg(sim, p->id, "is eating", 0);
	sleep_ms_stop(sim, sim->eat_ms);

	/* 放下叉子：顺序一般无所谓，但必须都 unlock */
	pthread_mutex_unlock(sec);
	pthread_mutex_unlock(first);
}

/* =========================
 * philo_thread
 * 作用：哲学家线程主循环
 * 行为：不断“吃 -> 睡 -> 想”，直到 stop=1
 *
 * 参数：arg - 传进来的 t_philo* 指针
 * 返回：NULL
 * ========================= */
void	*philo_thread(void *arg)
{
	t_philo	*p;
	t_sim	*sim;

	/* 把 void* 转回哲学家指针 */
	p = (t_philo *)arg;
	sim = p->sim;

	/* 小技巧：偶数号先稍微延迟一下再开始
	 * 目的：错开大家抢叉子的时刻，减少全员同时竞争的峰值
	 */
	if ((p->id % 2) == 0)
		usleep(500);

	/* 主循环：只要没 stop 就继续 */
	while (!stop_get(sim))
	{
		/* 吃一次 */
		eat_once(p);

		/* 吃完可能刚好 stop（比如别人死了），那就别再打印睡觉了 */
		if (stop_get(sim))
			break ;

		/* 睡觉 */
		log_msg(sim, p->id, "is sleeping", 0);
		sleep_ms_stop(sim, sim->sleep_ms);

		/* 思考 */
		log_msg(sim, p->id, "is thinking", 0);

		/* 再稍微让出 CPU 一点点，避免忙等太凶 */
		usleep(200);
	}
	return (NULL);
}

/* =========================
 * start_philos
 * 作用：创建所有哲学家线程
 *
 * 参数：
 * - sim：全局模拟器
 * - ph：哲学家数组（每个元素给一个线程）
 * - th：线程句柄数组（保存 pthread_create 返回的 thread id）
 *
 * 返回：
 * - 0：成功创建全部线程
 * - 1：失败（并会 stop + join 已创建的线程）
 * ========================= */
int	start_philos(t_sim *sim, t_philo *ph, pthread_t *th)
{
	int	i;

	i = 0;
	while (i < sim->count)
	{
		/* 创建第 i 个哲学家线程，入口函数 philo_thread，参数是 &ph[i] */
		if (pthread_create(&th[i], NULL, philo_thread, &ph[i]) != 0)
		{
			/* 一旦失败：
			 * 1) 立刻 stop，告诉已创建线程退出
			 * 2) join 回收已创建线程（数量是 i）
			 * 3) 返回错误
			 */
			stop_set(sim);
			join_philos(th, i);
			return (print_err("philo thread failed"));
		}
		i++;
	}
	return (0);
}

/* =========================
 * join_philos
 * 作用：等待 n 个哲学家线程结束（join）
 *
 * 参数：
 * - th：线程数组
 * - n：要 join 的数量（可能小于总数，用于“创建中途失败”的场景）
 * ========================= */
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
