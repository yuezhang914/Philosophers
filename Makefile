# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: yzhang2 <yzhang2@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/16 00:16:48 by yzhang2           #+#    #+#              #
#    Updated: 2026/01/30 14:38:28 by yzhang2          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


NAME	=	philo

CC		=	cc
CFLAGS	=	-Wall -Wextra -Werror -g3 -pthread

SRC_DIR	=	src
OBJ_DIR	=	obj

INCLUDE	=	-I .
HEADER	=	philo.h

SRCS	=	main.c $(wildcard $(SRC_DIR)/*.c)
OBJS	=	$(SRCS:%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(INCLUDE) -o $(NAME)

$(OBJ_DIR)/%.o: %.c $(HEADER) Makefile
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
