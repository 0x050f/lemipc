#ifndef LEMIPC_H
# define LEMIPC_H

# define _GNU_SOURCE
# include <errno.h>
# include <signal.h>
# include <stdbool.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/ipc.h>
# include <sys/msg.h>
# include <sys/sem.h>
# include <sys/shm.h>
# include <sys/stat.h>
# include <string.h>
# include <time.h>
# include <unistd.h>


# define PRG_NAME	"lemipc"

struct			player
{
	pid_t		pid;
	int			team;
	int			pos_x;
	int			pos_y;
};

# define CHAT_HEIGHT	10
# define CHAT_WIDTH		50

# define PLAYERS		0
# define MAP			1
# define PLAY			2

# define UP				0
# define DOWN			1
# define LEFT			2
# define RIGHT			3

struct			ipc
{
	int				shm_id;
	int				sem_id[3];
	int				mq_id;
	struct game		*game;
	struct player	player;
	uint8_t			*chatbox;
};

# define HEIGHT			4
# define WIDTH			10

# define MAX_PLAYERS	HEIGHT * WIDTH

struct			game
{
	pid_t				pid_turn;
	int					nb_players;
	struct player		players[MAX_PLAYERS];
	uint8_t				map[HEIGHT][WIDTH];
};

/* screen.c */
void	show_map(struct game *game, struct player *player);
void	show_chatbox(uint8_t *chatbox);
void	append_msg_chatbox(uint8_t *chatbox, char *msg, size_t size);
void	show_game(struct ipc *ipc);

/* game.c */
int		create_game(struct ipc *ipc);
int		setup_chatbox(struct ipc *ipc);
int		join_game(struct ipc *ipc);
int		exit_game(struct ipc *ipc);

struct ipc_msgbuf
{
	long		mtype;
	pid_t		mpid;
	char		mtext[256];
};

/* msg.c */
void	recv_msg(struct ipc *ipc, char buff[256]);
void	send_msg_pid(struct ipc *ipc, pid_t pid, char *msg);
void	send_msg_self(struct ipc *ipc, char *msg);
void	send_msg_team(struct ipc *ipc, char *msg);
void	send_msg_broadcast(struct ipc *ipc, char *msg);

/* utils.c */
int		sem_lock(int sem_id);
int		sem_trylock(int sem_id);
int		sem_tryunlock(int sem_id);
int		sem_unlock(int sem_id);

#endif
