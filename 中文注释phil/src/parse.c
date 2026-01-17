/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:09:40 by yzhang2           #+#    #+#             */
/*   Updated: 2026/01/17 19:37:29 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* =========================
 * is_uint_str
 * 作用：检查字符串是否“只包含数字字符 0~9”
 * - 空串也算不合法（必须至少有一个数字）
 *
 * 参数：s - 输入字符串
 * 返回：1 表示是纯数字；0 表示不是
 * ========================= */
static int	is_uint_str(const char *s)
{
	int	i;

	i = 0;
	if (!s || !s[0])
		return (0);
	while (s[i])
	{
		if (s[i] < '0' || s[i] > '9')
			return (0);
		i++;
	}
	return (1);
}

/* =========================
 * to_long
 * 作用：把“纯数字字符串”安全转换成 long
 * - 手动计算 num = num*10 + digit
 * - 同时做溢出检查：如果下一步会超过 LONG_MAX，就返回失败
 *
 * 参数：
 * - s：数字字符串（假设已是纯数字）
 * - out：输出参数，返回转换结果
 *
 * 返回：1 成功；0 失败（溢出）
 * ========================= */
static int	to_long(const char *s, long *out)
{
	long	num;
	int		d;

	num = 0;
	while (*s)
	{
		d = *s - '0';

		/* 溢出判断：
		 * 如果 num*10 + d > LONG_MAX
		 * 等价于：num > (LONG_MAX - d) / 10
		 */
		if (num > (LONG_MAX - d) / 10)
			return (0);

		num = num * 10 + d;
		s++;
	}
	*out = num;
	return (1);
}

/* =========================
 * parse_pos_int
 * 作用：把字符串解析成“正整数 int”，并要求范围 1..INT_MAX
 *
 * 参数：
 * - s：输入字符串
 * - dst：输出参数，返回 int 值
 *
 * 返回：1 成功；0 失败（非数字/溢出/范围不对）
 * ========================= */
static int	parse_pos_int(const char *s, int *dst)
{
	long	val;

	/* 必须是纯数字 */
	if (!is_uint_str(s))
		return (0);

	/* 必须能转 long 且不溢出 */
	if (!to_long(s, &val))
		return (0);

	/* 必须在 1..INT_MAX */
	if (val <= 0 || val > INT_MAX)
		return (0);

	*dst = (int)val;
	return (1);
}

/* =========================
 * sim_parse
 * 作用：解析命令行参数，填充 sim 的配置，并初始化一些字段
 *
 * 参数：
 * - argc/argv：命令行参数
 * - sim：输出参数，填充配置
 *
 * 返回：0 成功；1 失败
 * ========================= */
int	sim_parse(int argc, char **argv, t_sim *sim)
{
	/* 参数个数必须是 5 或 6：
	 * ./philo n die eat sleep [must_eat]
	 */
	if (argc != 5 && argc != 6)
		return (1);

	/* 逐个解析并写入 sim */
	if (!parse_pos_int(argv[1], &sim->count))
		return (1);
	if (!parse_pos_int(argv[2], &sim->die_ms))
		return (1);
	if (!parse_pos_int(argv[3], &sim->eat_ms))
		return (1);
	if (!parse_pos_int(argv[4], &sim->sleep_ms))
		return (1);

	/* must_eat 默认 -1：表示“不限制吃多少次” */
	sim->must_eat = -1;
	if (argc == 6 && !parse_pos_int(argv[5], &sim->must_eat))
		return (1);

	/* 初始化运行时状态 */
	sim->stop = 0;       /* 0 表示还没停止 */
	sim->start_ms = 0;   /* 之后在 init_philo 里设置 */

	/* 初始化“已初始化锁计数器” */
	sim->fork_inited = 0;
	sim->meal_inited = 0;
	sim->print_inited = 0;
	sim->state_inited = 0;

	/* forks 指针先置 NULL，避免 release 时误 free */
	sim->forks = NULL;
	return (0);
}
