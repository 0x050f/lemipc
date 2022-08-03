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
	int			x;
	int			y;
}				t_lemipc;

# define HEIGHT	10
# define WIDTH	35

# define CHAT_HEIGHT 10

# define END	0xdead

typedef struct	s_game
{
	sem_t		sem_player;
	sem_t		sem_map;
	int			nb_player;
	int			x_playing;
	int			y_playing;
	uint8_t		map[HEIGHT][WIDTH];
}				t_game;

#endif
