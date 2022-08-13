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

bool		check_if_empty(struct ipc *ipc, int x, int y)
{
	return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT &&
				ipc->game->map[y][x] == ' ');
}

int			get_dist(int x1, int y1, int x2, int y2)
{
	return (sqrt(pow(y2 - y1, 2) + pow(x2 - x1, 2)));
}

void		move_random(struct ipc *ipc)
{
	int				move = -1;
	int				new_pos_x, new_pos_y;
	struct game		*game;
	struct player	*player;

	game = ipc->game;
	player = &ipc->player;
	/* Check if can play otherwise pass */
	if (check_if_empty(ipc, player->pos_x + 1, player->pos_y) ||
	check_if_empty(ipc, player->pos_x, player->pos_y + 1) ||
	check_if_empty(ipc, player->pos_x - 1, player->pos_y) ||
	check_if_empty(ipc, player->pos_x, player->pos_y - 1))
	{
		new_pos_x = player->pos_x;
		new_pos_y = player->pos_y;
		while (move == -1)
		{
			move = rand() % 4;
			if (move == UP && check_if_empty(ipc, player->pos_x, player->pos_y - 1))
				new_pos_y--;
			else if (move == DOWN && check_if_empty(ipc, player->pos_x, player->pos_y + 1))
				new_pos_y++;
			else if (move == LEFT && check_if_empty(ipc, player->pos_x - 1, player->pos_y))
				new_pos_x--;
			else if (move == RIGHT && check_if_empty(ipc, player->pos_x + 1, player->pos_y))
				new_pos_x++;
			else
				move = -1;
		}
		game->map[player->pos_y][player->pos_x] = ' ';
		game->map[new_pos_y][new_pos_x] = player->team + '0';
		player->pos_x = new_pos_x;
		player->pos_y = new_pos_y;
	}
}

void		move(struct ipc *ipc, int x, int y)
{
	struct game		*game;
	struct player	*player;
	int				distance;

	game = ipc->game;
	player = &ipc->player;
	distance = get_dist(player->pos_x, player->pos_y, x, y);
	sem_lock(ipc->sem_id[MAP]);
	game->map[player->pos_y][player->pos_x] = ' ';
	if (check_if_empty(ipc, player->pos_x - 1, player->pos_y)
&& distance > get_dist(player->pos_x - 1, player->pos_y, x, y))
		player->pos_x--;
	else if (check_if_empty(ipc, player->pos_x, player->pos_y - 1)
&& distance > get_dist(player->pos_x, player->pos_y - 1, x, y))
		player->pos_y--;
	else if (check_if_empty(ipc, player->pos_x + 1, player->pos_y)
&& distance > get_dist(player->pos_x + 1, player->pos_y, x, y))
		player->pos_x++;
	else if (check_if_empty(ipc, player->pos_x, player->pos_y + 1)
&& distance > get_dist(player->pos_x, player->pos_y + 1, x, y))
		player->pos_y++;
	else
	{
		move_random(ipc);
		sem_unlock(ipc->sem_id[MAP]);
		return ;
	}
	game->map[player->pos_y][player->pos_x] = player->team + '0';
	sem_unlock(ipc->sem_id[MAP]);
}

bool		check_if_target(struct ipc *ipc, int x, int y)
{
	return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT &&
				ipc->game->map[y][x] != ' ' &&
				ipc->game->map[y][x] != ipc->player.team + '0');
}

int			get_closest_target(struct ipc *ipc, int *x, int *y)
{
	int max_dist = (HEIGHT - 1) + (WIDTH - 1);

	sem_lock(ipc->sem_id[MAP]);
	for (int d = 1; d < max_dist; d++)
	{
		for (int i = -d; i <= d; i++)
		{
			*x = ipc->player.pos_x + d - abs(i);
			*y = ipc->player.pos_y + i;
			if (check_if_target(ipc, *x, *y))
			{
				sem_unlock(ipc->sem_id[MAP]);
				return (EXIT_SUCCESS);
			}
			*x = ipc->player.pos_x - d + abs(i);
			if (check_if_target(ipc, *x, *y))
			{
				sem_unlock(ipc->sem_id[MAP]);
				return (EXIT_SUCCESS);
			}
		}
	}
	sem_unlock(ipc->sem_id[MAP]);
	return (EXIT_FAILURE);
}

int		play_game(struct ipc *ipc)
{
	int		locked = 0;
	int		nb_players;
	int		nb_teams;
	char	buf[256];

	show_game(ipc);
	nb_teams = count_nb_teams(ipc);
	while (nb_teams < 2)
	{
		recv_msg(ipc, NULL); // wake up when recv msg
		nb_teams = count_nb_teams(ipc);
	}
	while (nb_teams > 1)
	{
		show_game(ipc);
		if (sem_trylock(ipc->sem_id[PLAY]) == EXIT_SUCCESS)
		{
			locked = 1;
			sem_lock(ipc->sem_id[PLAYERS]);
			nb_players = ipc->game->nb_players;
			sem_unlock(ipc->sem_id[PLAYERS]);
			/* TODO: Get msg from team */
			int x, y;
			int target_x, target_y = -1;
			int team;
			for (size_t i = 0; i < (size_t)nb_players - 1; i++)
			{
				recv_msg(ipc, buf);
				if (sscanf(buf, "%d: attack (x: %d, y: %d)", &team, &x, &y) == 3)
				{
					if (team == ipc->player.team)
					{
						target_x = x;
						target_y = x;
					}
				}
			}
			ipc->game->pid_turn = ipc->player.pid;
			if ((target_x == -1 || target_y == -1) &&
get_closest_target(ipc, &target_x, &target_y) == EXIT_FAILURE)
				sprintf(buf, "Player from team %d didn't moved");
			else
			{
				move(ipc, target_x, target_y);
				sprintf(buf, "Player from team %d moved (x: %d, y: %d)", ipc->player.team, ipc->player.pos_x, ipc->player.pos_y);
			}
			send_msg_broadcast(ipc, buf);
		}
		else
		{
			locked = 0;
			int x, y;
			get_closest_target(ipc, &x, &y);
			sprintf(buf, "attack (x: %d, y: %d)", x, y);
			send_msg_team(ipc, buf);
			sem_lock(ipc->sem_id[PLAYERS]);
			nb_players = ipc->game->nb_players;
			sem_unlock(ipc->sem_id[PLAYERS]);
			for (size_t i = 0; i < (size_t)nb_players - 1; i++)
				recv_msg(ipc, NULL);
		}
		if (is_circle(ipc))
		{
			sprintf(buf, "I'm dead !");
			send_msg_team(ipc, buf);
			show_game(ipc);
			exit_game(ipc);
			for (size_t i = 0; i < (size_t)nb_players - 1; i++)
				recv_msg(ipc, NULL);
			return(EXIT_SUCCESS);
		}
		sprintf(buf, "Ready for next turn !");
		send_msg_team(ipc, buf);
		for (size_t i = 0; i < (size_t)nb_players - 1; i++)
			recv_msg(ipc, NULL);
		if (locked)
			sem_unlock(ipc->sem_id[PLAY]);
		usleep(100000);
		nb_teams = count_nb_teams(ipc);
	}
	sprintf(buf, "You win !");
	send_msg_self(ipc, buf);
	show_game(ipc);
	exit_game(ipc);
	return (EXIT_SUCCESS);
}

int		setup_chatbox(struct ipc *ipc)
{
	size_t chatbox_size = (CHAT_HEIGHT - 1) * (CHAT_WIDTH - 1) * sizeof(uint8_t);
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
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	char			buf[256];
	struct game		*game;
	struct player	*player;
	int				nb_players;
	int				nb_teams;
	time_t			t;

	srand((unsigned) time(&t));
	player = &ipc->player;
	game = ipc->game;
	player->pid = getpid();
	sem_lock(ipc->sem_id[PLAY]);
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
	nb_players = game->nb_players;
	sem_unlock(ipc->sem_id[PLAYERS]);
	nb_teams = count_nb_teams(ipc);
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
	if (nb_teams > 2)
	{
		for (size_t i = 0; i < (size_t)nb_players - 1; i++)
			recv_msg(ipc, NULL);
	}
	sprintf(buf, "Player joined team %d", player->team);
	send_msg_broadcast(ipc, buf);
	sem_unlock(ipc->sem_id[PLAY]);
	return (play_game(ipc));
}

int		remove_player(struct ipc *ipc)
{
	struct game		*game;
	int				nb_players;

	game = ipc->game;
	sem_lock(ipc->sem_id[PLAYERS]);
	sem_lock(ipc->sem_id[MAP]);
	if (game->nb_players)
		game->nb_players--;
	nb_players = game->nb_players;
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i].pid != ipc->player.pid)
		i++;
	if (i != MAX_PLAYERS)
		game->players[i].pid = -1;
	ipc->game->map[ipc->player.pos_y][ipc->player.pos_x] = ' ';
	sem_unlock(ipc->sem_id[MAP]);
	sem_unlock(ipc->sem_id[PLAYERS]);
	return (nb_players);
}

int		exit_game(struct ipc *ipc)
{
	int nb_players;

	nb_players = remove_player(ipc);
	if (!nb_players) // DELETE IPCS
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
	shmdt(ipc->game);
	free(ipc->chatbox);
	ipc->game = NULL;
	ipc->chatbox = NULL;
	return (EXIT_SUCCESS);
}
