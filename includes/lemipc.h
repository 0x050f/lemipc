#ifndef LEMIPC_H
# define LEMIPC_H

# include <errno.h>
# include <fcntl.h>
# include <stdbool.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <unistd.h>

# define PRG_NAME "lemipc"

typedef struct	s_lemipc {
	int			fd;
	size_t		size;
	void		*addr;
}				t_lemipc;

typedef struct	s_game {
	uint8_t		map[10][10];
}				t_game;

#endif
