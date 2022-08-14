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
		if (game->players[i].pid != -1 && game->players[i].team != -1)
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
