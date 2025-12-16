/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:09:40 by yzhang2           #+#    #+#             */
/*   Updated: 2025/12/16 01:07:21 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

/* 检查字符串是不是只包含数字 */
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

/* 把数字字符串转成 long，同时检查溢出 */
static int	to_long(const char *s, long *out)
{
	long	num;
	int		d;

	num = 0;
	while (*s)
	{
		d = *s - '0';
		if (num > (LONG_MAX - d) / 10)
			return (0);
		num = num * 10 + d;
		s++;
	}
	*out = num;
	return (1);
}

/* 解析一个正整数到 int，要求 1..INT_MAX */
static int	parse_pos_int(const char *s, int *dst)
{
	long	val;

	if (!is_uint_str(s))
		return (0);
	if (!to_long(s, &val))
		return (0);
	if (val <= 0 || val > INT_MAX)
		return (0);
	*dst = (int)val;
	return (1);
}

/* 解析命令行参数，填充 sim 的基本配置 */
int	sim_parse(int argc, char **argv, t_sim *sim)
{
	if (argc != 5 && argc != 6)
		return (1);
	if (!parse_pos_int(argv[1], &sim->count))
		return (1);
	if (!parse_pos_int(argv[2], &sim->die_ms))
		return (1);
	if (!parse_pos_int(argv[3], &sim->eat_ms))
		return (1);
	if (!parse_pos_int(argv[4], &sim->sleep_ms))
		return (1);
	sim->must_eat = -1;
	if (argc == 6 && !parse_pos_int(argv[5], &sim->must_eat))
		return (1);
	sim->stop = 0;
	sim->start_ms = 0;
	sim->fork_inited = 0;
	sim->meal_inited = 0;
	sim->print_inited = 0;
	sim->state_inited = 0;
	sim->forks = NULL;
	return (0);
}
