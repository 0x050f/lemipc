#ifndef LEMIPC_H
# define LEMIPC_H

# include <errno.h>
# include <fcntl.h>
# include <mqueue.h>
# include <semaphore.h>
# include <signal.h>
# include <stdbool.h>
# include <stdlib.h>
# include <stdint.h>
# include <stdio.h>
# include <string.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <time.h>
# include <unistd.h>

# define PRG_NAME "lemipc"

typedef struct	s_lemipc
{
	int			shm_fd;
	int			mq_fd;
	size_t		size;
	void		*addr;
	int			pos_x;
	int			pos_y;
	uint8_t		*chatbox;
}				t_lemipc;

# define HEIGHT	10
# define WIDTH	35

# define CHAT_HEIGHT 10

# define END	0xdead

typedef struct	s_game
{
	sem_t		sem_game;
	int			nb_players;
	int			pos_x_turn;
	int			pos_y_turn;
	uint8_t		map[HEIGHT][WIDTH];
}				t_game;

extern t_lemipc	g_lemipc;

/* game.c */
int			create_game(int fd);
int			join_game(int shm_fd, int mq_fd, size_t size, int team_number);
int			exit_game(t_game *game, size_t size);

/* utils.c */
size_t		align_up(size_t size, size_t align);

#endif
