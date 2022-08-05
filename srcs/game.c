#include "lemipc.h"

bool		is_circle(struct ipc *ipc)
{
	struct player	*player;
	struct game		*game;
	int				team_nb[10];

	game = ipc->game;
	player = &ipc->player;
	memset(team_nb, 0, sizeof(team_nb));
	sem_lock(ipc->sem_id[MAP]);
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			if (player->pos_x + x >= 0 && player->pos_x + x < WIDTH &&
				player->pos_y + y >= 0 && player->pos_y + y < HEIGHT &&
				game->map[player->pos_y + y][player->pos_x + x] != ' ')
			{
				int nb = game->map[player->pos_y + y][player->pos_x + x] - '0';
				team_nb[nb]++;
			}
		}
	}
	for (size_t i = 0; i < 10; i++)
	{
		if (i != (size_t)player->team && team_nb[i] > 1)
		{
			game->map[player->pos_y][player->pos_x] = ' ';
			sem_unlock(ipc->sem_id[MAP]);
			return (true);
		}
	}
	sem_unlock(ipc->sem_id[MAP]);
	return (false);
}

int		count_nb_teams(struct ipc *ipc)
{
	int				sum;
	int				team_nb[10];
	struct game		*game;

	sem_lock(ipc->sem_id[PLAYERS]);
	memset(team_nb, 0, sizeof(team_nb));
	game = ipc->game;
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (game->players[i].pid != -1)
			team_nb[game->players[i].team]++;
	}
	sum = 0;
	for (size_t i = 0; i < 10; i++)
	{
		if (team_nb[i])
			sum++;
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
	return (sum);
}

void	move(struct ipc *ipc)
{
	int				move = -1;
	struct game		*game;
	struct player	*player;
	int				new_pos_x;
	int				new_pos_y;

	game = ipc->game;
	player = &ipc->player;
	sem_lock(ipc->sem_id[MAP]);
	/* Check if can play otherwise pass */
	if ((player->pos_x + 1 < WIDTH && game->map[player->pos_y][player->pos_x + 1] == ' ') ||
	(player->pos_y + 1 < HEIGHT && game->map[player->pos_y + 1][player->pos_x] == ' ') ||
	(player->pos_x - 1 >= 0 && game->map[player->pos_y][player->pos_x - 1] == ' ') ||
	(player->pos_y - 1 >= 0 && game->map[player->pos_y - 1][player->pos_x] == ' '))
	{
		new_pos_x = player->pos_x;
		new_pos_y = player->pos_y;
		while (move == -1)
		{
			move = rand() % 4;
			if (move == UP && player->pos_y - 1 >= 0 && game->map[player->pos_y - 1][player->pos_x] == ' ')
				new_pos_y = player->pos_y - 1;
			else if (move == DOWN && player->pos_y + 1 < HEIGHT && game->map[player->pos_y + 1][player->pos_x] == ' ')
				new_pos_y = player->pos_y + 1;
			else if (move == LEFT && player->pos_x - 1 >= 0 && game->map[player->pos_y][player->pos_x - 1] == ' ')
				new_pos_x = player->pos_x - 1;
			else if (move == RIGHT && player->pos_x + 1 < WIDTH && game->map[player->pos_y][player->pos_x + 1] == ' ')
				new_pos_x = player->pos_x + 1;
			else
				move = -1;
		}
		game->map[player->pos_y][player->pos_x] = ' ';
		game->map[new_pos_y][new_pos_x] = player->team + '0';
		player->pos_x = new_pos_x;
		player->pos_y = new_pos_y;
	}
	sem_unlock(ipc->sem_id[MAP]);
}

int		play_game(struct ipc *ipc)
{
	while ((count_nb_teams(ipc)) < 2)
	{
		recv_msg(ipc); // wake up when recv msg
		show_game(ipc);
	}
	while (count_nb_teams(ipc) > 1)
	{
		if (sem_trylock(ipc->sem_id[PLAY]) == EXIT_SUCCESS)
		{
			usleep(10000);
			sem_lock(ipc->sem_id[MAP]);
			memcpy(&ipc->game->player_turn, &ipc->player, sizeof(struct player));
			sem_unlock(ipc->sem_id[MAP]);
			send_msg_self(ipc, "SYSTEM: Your turn");
			recv_msg(ipc);
			show_game(ipc);
			while (check_recv_msg(ipc) == EXIT_SUCCESS)
				show_game(ipc);
			move(ipc);
			char buf[256];
			sprintf(buf, "Player from team %d moved", ipc->player.team);
			send_msg_broadcast(ipc, buf);
			recv_msg(ipc);
			show_game(ipc);
			while (check_recv_msg(ipc) == EXIT_SUCCESS)
				show_game(ipc);
			sem_unlock(ipc->sem_id[PLAY]);
		}
		else
		{
			/* TODO: find target */
			recv_msg(ipc);
			show_game(ipc);
			if (is_circle(ipc))
				return(EXIT_SUCCESS);
		}
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
	printf("You win !\n");
	return (EXIT_SUCCESS);
}

int		setup_chatbox(struct ipc *ipc)
{
	size_t chatbox_size = (CHAT_HEIGHT - 1) * ((WIDTH * 2) - 1) * sizeof(uint8_t);
	ipc->chatbox = malloc(chatbox_size);
	if (!ipc->chatbox)
	{
		dprintf(STDERR_FILENO, "%s: malloc(): %s\n", PRG_NAME, strerror(errno));
		shmdt(ipc->game);
		return (EXIT_FAILURE);
	}
	memset(ipc->chatbox, ' ', chatbox_size);
	return (EXIT_SUCCESS);
}

int		create_game(struct ipc *ipc)
{
	ipc->game = shmat(ipc->shm_id, NULL, 0);
	ipc->game->nb_players = 0;
	memset(ipc->game->players, -1, sizeof(ipc->game->players));
	memset(ipc->game->map, ' ', sizeof(ipc->game->map));
	ipc->game->player_turn.pos_x = -1;
	ipc->game->player_turn.pos_y = -1;
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	char			buf[256];
	struct game		*game;
	struct player	*player;
	time_t			t;

	srand((unsigned) time(&t));
	player = &ipc->player;
	game = ipc->game;
	player->pid = getpid();
	sem_lock(ipc->sem_id[PLAYERS]);
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i].pid != -1)
		i++;
	if (i == MAX_PLAYERS)
	{
		sem_unlock(ipc->sem_id[PLAYERS]);
		dprintf(STDERR_FILENO, "Game is full\n");
		return (EXIT_FAILURE);
	}
	game->nb_players++;
	sem_unlock(ipc->sem_id[PLAYERS]);
	sem_lock(ipc->sem_id[MAP]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	do {
		player->pos_x = rand() % WIDTH;
		player->pos_y = rand() % HEIGHT;
	}
	while (game->map[player->pos_y][player->pos_x] != ' ');
	game->map[player->pos_y][player->pos_x] = player->team + '0';
	memcpy(&game->players[i], player, sizeof(struct player));
	sem_unlock(ipc->sem_id[MAP]);
	sprintf(buf, "Player joined team %d", player->team);
	send_msg_broadcast(ipc, buf);
	return (play_game(ipc));
}

int		exit_game(struct ipc *ipc)
{
	char			buffer[256];
	struct game		*game;
	pid_t pid;

	game = ipc->game;
	pid = getpid();
	sem_tryunlock(ipc->sem_id[MAP]);
	sem_tryunlock(ipc->sem_id[PLAYERS]);
	sem_tryunlock(ipc->sem_id[PLAY]);
	sem_lock(ipc->sem_id[PLAYERS]);
	if (game->nb_players)
		game->nb_players--;
	int nb_players = game->nb_players;
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i].pid != pid)
		i++;
	if (i != MAX_PLAYERS)
		game->players[i].pid = -1;
	sem_unlock(ipc->sem_id[PLAYERS]);
	sprintf(buffer, "Player left team %d", ipc->player.team);
	send_msg_broadcast(ipc, buffer);
	sem_lock(ipc->sem_id[MAP]);
	ipc->game->map[ipc->player.pos_y][ipc->player.pos_x] = ' ';
	if (!nb_players)
	{
		shmdt(ipc->game);
		msgctl(ipc->mq_id, IPC_RMID, 0);
		semctl(ipc->sem_id[MAP], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAYERS], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAY], IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		free(ipc->chatbox);
		return (EXIT_SUCCESS);
	}
	sem_unlock(ipc->sem_id[MAP]);
	shmdt(ipc->game);
	free(ipc->chatbox);
	return (EXIT_SUCCESS);
}
