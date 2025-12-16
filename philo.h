/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   philo.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 00:00:00 by yzhang2           #+#    #+#             */
/*   Updated: 2025/12/16 00:09:27 by yzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PHILO_H
# define PHILO_H

# include <limits.h>
# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/time.h>
# include <unistd.h>

typedef struct s_sim
{
	int				count;
	int				die_ms;
	int				eat_ms;
	int				sleep_ms;
	int				must_eat;

	int				stop;
	long			start_ms;

	int				fork_inited;
	int				meal_inited;
	int				print_inited;
	int				state_inited;

	pthread_mutex_t	*forks;
	pthread_mutex_t	print_lock;
	pthread_mutex_t	state_lock;
}					t_sim;

typedef struct s_philo
{
	int				id;
	int				meals;
	long			last_meal;

	pthread_mutex_t	meal_lock;
	pthread_mutex_t	*left;
	pthread_mutex_t	*right;
	t_sim			*sim;
}					t_philo;

int					sim_parse(int argc, char **argv, t_sim *sim);

int					sim_init_mutex(t_sim *sim);
int					sim_init_philo(t_sim *sim, t_philo *ph);

long				time_ms(void);
void				sleep_ms(long ms);
void				sleep_ms_stop(t_sim *sim, long ms);

int					stop_get(t_sim *sim);
void				stop_set(t_sim *sim);
void				log_msg(t_sim *sim, int id, const char *msg, int force);

void				*philo_thread(void *arg);
int					start_philos(t_sim *sim, t_philo *ph, pthread_t *th);
void				join_philos(pthread_t *th, int n);

void				*watch_thread(void *arg);

int					print_err(const char *msg);
void				sim_release(t_sim *sim, t_philo *ph, pthread_t *th);

#endif
