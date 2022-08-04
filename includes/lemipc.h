#ifndef LEMIPC_H
# define LEMIPC_H

# include <errno.h>
# include <signal.h>
# include <stdbool.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/ipc.h>
# include <sys/sem.h>
# include <sys/shm.h>
# include <sys/stat.h>
# include <string.h>
# include <time.h>
# include <unistd.h>

# define PRG_NAME	"lemipc"

struct			player
{
	int			team;
	int			pos_x;
	int			pos_y;
};

# define CHAT_HEIGHT 10

struct			ipc
{
	int				shm_id;
	int				sem_id;
	struct game		*game;
	struct player	player;
	uint8_t			*chatbox;
};

# define HEIGHT			10
# define WIDTH			35

# define MAX_PLAYERS	20

struct			game
{
	int					nb_players;
	pid_t				players[MAX_PLAYERS];
	struct player		player_turn;
	uint8_t				map[HEIGHT][WIDTH];
};

/* screen.c */
void	show_map(struct game *game, struct player *player);
void	show_chatbox(uint8_t *chatbox);
void	append_msg_chatbox(uint8_t *chatbox, char *msg, size_t size);
void	show_game(struct ipc *ipc);

/* game.c */
int		create_game(struct ipc *ipc);
int		join_game(struct ipc *ipc);
int		exit_game(struct ipc *ipc);

/* utils.c */
int		sem_lock(int sem_id);
int		sem_unlock(int sem_id);

#endif
