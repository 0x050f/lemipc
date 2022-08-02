#ifndef LEMIPC_H
# define LEMIPC_H

# include <errno.h>
# include <fcntl.h>
# include <mqueue.h>
# include <signal.h>
# include <stdbool.h>
# include <stdlib.h>
# include <stdint.h>
# include <stdio.h>
# include <string.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <unistd.h>

# define PRG_NAME "lemipc"

typedef struct	s_lemipc
{
	int			shm_fd;
	int			mq_fd;
	size_t		size;
	void		*addr;
}				t_lemipc;

# define HEIGHT	25
# define WIDTH	50

typedef struct	s_game
{
	int			nb_player;
	uint8_t		map[HEIGHT][WIDTH];
}				t_game;

#endif
