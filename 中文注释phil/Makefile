# 生成的可执行文件名
NAME	=	philo

# 编译器
CC		=	cc

# 编译参数：
# -Wall -Wextra -Werror：42 常规要求，开更多警告并当错误处理
# -g3：带调试信息（方便 gdb/valgrind）
# -pthread：必须！否则 pthread_create/join/mutex 相关可能链接失败
CFLAGS	=	-Wall -Wextra -Werror -g3 -pthread

SRC_DIR	=	src
OBJ_DIR	=	obj

# 头文件路径：-I . 表示在当前目录找 philo.h
INCLUDE	=	-I .
HEADER	=	philo.h

# SRCS：
# - main.c
# - src/ 下面所有 .c（wildcard 自动收集）
SRCS	=	main.c $(wildcard $(SRC_DIR)/*.c)

# OBJS：把每个 .c 映射到 obj/ 下面对应的 .o
OBJS	=	$(SRCS:%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

# 链接阶段：把所有 .o 链接成可执行文件 philo
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(INCLUDE) -o $(NAME)

# 编译阶段：把 %.c 编译成 obj/%.o
# 依赖：源文件 + 头文件 + Makefile（任意变化都会触发重编）
$(OBJ_DIR)/%.o: %.c $(HEADER) Makefile
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
