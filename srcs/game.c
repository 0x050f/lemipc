#include "lemipc.h"

int		waiting_game(struct ipc *ipc)
{
	int nb_teams;

	nb_teams = count_nb_teams(ipc);
	while (nb_teams < 2)
	{
		recv_msg(ipc, NULL); // wake up when recv msg
		nb_teams = count_nb_teams(ipc);
	}
	return (nb_teams);
}

int		play_game(struct ipc *ipc)
{
	int		locked = 0;
	int		nb_players;
	int		nb_teams;
	char	buf[256];

	show_game(ipc);
	nb_teams = waiting_game(ipc);
	while (nb_teams > 1)
	{
		show_game(ipc);
		if (sem_trylock(ipc->sem_id[PLAY]) == EXIT_SUCCESS)
		{
			locked = 1;
			sem_lock(ipc->sem_id[PLAYERS]);
			nb_players = ipc->game->nb_players;
			sem_unlock(ipc->sem_id[PLAYERS]);
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
						target_y = y;
					}
				}
			}
			ipc->game->pid_turn = ipc->player.pid;
			if ((target_x == -1 || target_y == -1))
				sprintf(buf, "Player from team %d didn't moved", ipc->player.team);
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
			sem_lock(ipc->sem_id[PLAYERS]);
			for (size_t i = 0; i < (size_t)nb_players - 1; i++)
			{
				if (ipc->game->players[i].pid == ipc->player.pid)
					ipc->game->players[i].team = -1;
			}
			sem_unlock(ipc->sem_id[PLAYERS]);
			sprintf(buf, "I'm dead !");
			send_msg_team(ipc, buf);
			for (size_t i = 0; i < (size_t)nb_players - 1; i++)
				recv_msg(ipc, NULL);
			sprintf(buf, "You lose !");
			send_msg_self(ipc, buf);
			show_game(ipc);
			return(EXIT_SUCCESS);
		}
		sprintf(buf, "Ready for next turn !");
		show_game(ipc);
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
	{
		game->players[i].pid = -1;
		game->players[i].team = -1;
		ipc->game->map[ipc->player.pos_y][ipc->player.pos_x] = ' ';
	}
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
