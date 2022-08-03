#include "lemipc.h"

int			create_game(int fd)
{
	printf("Creating the game...\n");
	g_lemipc.size = align_up(sizeof(t_game), getpagesize());
	if (ftruncate(fd, g_lemipc.size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: ftruncate(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	g_lemipc.addr = mmap(0, g_lemipc.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (g_lemipc.addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	printf("game: %p\n", g_lemipc.addr);
	printf("size: %ld\n", g_lemipc.size);
	/* Init mem */
	memset(g_lemipc.addr, 0, g_lemipc.size);
	/* Init map */
	t_game *game = g_lemipc.addr;
	if (sem_init(&game->sem_player, 1, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: sem_init(): %s\n", PRG_NAME, strerror(errno));
		munmap(g_lemipc.addr, g_lemipc.size);
		return (EXIT_FAILURE);
	}
	if (sem_wait(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	if (sem_init(&game-> sem_map, 1, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: sem_init(): %s\n", PRG_NAME, strerror(errno));
		sem_destroy(&game->sem_player);
		munmap(g_lemipc.addr, g_lemipc.size);
		return (EXIT_FAILURE);
	}
	if (sem_wait(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	game->nb_player = 0;
	game->pos_x_turn = -1;
	game->pos_y_turn = -1;
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
			game->map[y][x] = ' ';
	}
	if (sem_post(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	if (sem_post(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	return (g_lemipc.size);
}

void		show_map(t_game *game)
{
	printf("\033[2J\033[H"); /* clear screen + top */
	printf("x: %d - y: %d\n", g_lemipc.pos_x, g_lemipc.pos_y);
	for (size_t x = 0; x <= WIDTH * 2; x++)
	{
		if (x == 0)
			printf("┌");
		else if (x == WIDTH * 2)
			printf("┐");
		else if (!(x % 2))
			printf("┬");
		else
			printf("─");
	}
	printf("\n");
	for (size_t y = 0; y < HEIGHT; y++)
	{
		printf("│");
		for (size_t x = 0; x < WIDTH; x++)
		{
			if (x == (size_t)game->pos_x_turn && y == (size_t)game->pos_y_turn)
				printf("\e[5m", game->map[y][x]);
			if (x == (size_t)g_lemipc.pos_x && y == (size_t)g_lemipc.pos_y)
				printf("\e[31m", game->map[y][x]);
			printf("%c\e[0m│", game->map[y][x]);
		}
		printf("\n");
		for (size_t x = 0; x <= WIDTH * 2; x++)
		{
			if (x == 0)
			{
				if (y != HEIGHT - 1)
					printf("├");
				else
					printf("└");
			}
			else if (x == WIDTH * 2)
			{
				if (y != HEIGHT - 1)
					printf("┤");
				else
					printf("┘");
			}
			else if (!(x % 2))
			{
				if (y != HEIGHT - 1)
					printf("┼");
				else
					printf("┴");
			}
			else
				printf("─");
		}
		printf("\n");
	}
	printf("\n");
}

// TODO: param chat
void		show_chatbox()
{
	for (size_t y = 0; y < CHAT_HEIGHT; y++)
	{
		for (size_t x = 0; x <= WIDTH * 2; x++)
		{
			if (x == 0 && y == 0)
				printf("┌");
			else if (x == WIDTH * 2 && y == 0)
				printf("┐");
			else if (x == 0 && y == CHAT_HEIGHT - 1)
				printf("└");
			else if (x == WIDTH * 2 && y == CHAT_HEIGHT - 1)
				printf("┘");
			else if (x == 0 || x == WIDTH * 2)
				printf("│");
			else if (y == 0 || y == CHAT_HEIGHT - 1)
				printf("─");
			else // TODO: PRINT CHAT
				printf(" ");
		}
		printf("\n");
	}
}


int			count_nb_team(t_game *game)
{
	int		teams[10];

	memset(teams, 0, sizeof(teams));
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
		{
			if (game->map[y][x] != ' ')
				teams[game->map[y][x] - '0']++;
		}
	}
	int sum = 0;
	for (size_t i = 0; i < 10; i++)
	{
		if (teams[i])
			sum++;
	}
	return (sum);
}

int			join_game(int shm_fd, int mq_fd, size_t size, int team_number)
{
	(void)mq_fd;
	g_lemipc.size = size;
	if (!g_lemipc.addr)
	{
		g_lemipc.addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (g_lemipc.addr == MAP_FAILED)
		{
			dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
			return (EXIT_FAILURE);
		}
	}
	t_game *game = g_lemipc.addr;

	show_map(game);
	show_chatbox();
	printf("Waiting to join the game...\n");
	if (sem_wait(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	game->nb_player++;
	if (sem_post(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	printf("Joined as player !\n");
	// TODO: show chat and map and enter in room

	if (sem_wait(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	printf("lol\n");
	time_t t;

	srand((unsigned) time(&t));
	do {
		g_lemipc.pos_x = rand() % WIDTH;
		g_lemipc.pos_y = rand() % HEIGHT;
	}
	while (game->map[g_lemipc.pos_y][g_lemipc.pos_x] != ' ');
	game->map[g_lemipc.pos_y][g_lemipc.pos_x] = team_number + '0';
	if (sem_post(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	while (count_nb_team(game) < 2 || game->nb_player < 4)
	{
		show_map(game);
		show_chatbox();
		printf("Waiting... (Min 2 teams and 4 players)\n");
		sleep(1);
	}
	show_map(game);
	show_chatbox();
	while (1)
	{
		struct timespec time;
		clock_gettime(CLOCK_REALTIME, &time);
		time.tv_sec += 1;
		if (sem_timedwait(&game->sem_map, &time) < 0)
		{
			show_map(game);
			show_chatbox();
		}
		else
		{
			printf("Your turn !\n");
			struct mq_attr attr;
			if (mq_getattr(mq_fd, &attr) < 0)
				dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
			char	buffer[attr.mq_msgsize];
			int		ret = read(STDIN_FILENO, buffer, sizeof(buffer));
			(void)ret;
			if (sem_post(&game->sem_map) < 0)
				dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		}
		/*
		struct mq_attr attr;
		if (mq_getattr(mq_fd, &attr) < 0)
			dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
		printf("mq_flags: %ld\n", attr.mq_flags);
		printf("mq_maxmsg: %ld\n", attr.mq_maxmsg);
		printf("mq_msgsize: %ld\n", attr.mq_msgsize);
		printf("mq_cur: %ld\n", attr.mq_curmsgs);
		int ret;
		char	buffer[attr.mq_msgsize];
		printf("before read\n");
		ret = read(STDIN_FILENO, buffer, sizeof(buffer));
		printf("after read\n");
		if (mq_send(mq_fd, buffer, ret, 0) < 0)
			dprintf(STDERR_FILENO, "%s: mq_send(): %s\n", PRG_NAME, strerror(errno));
		*/
	}
	return (exit_game(game, size));
}

int		exit_game(t_game *game, size_t size)
{
	int ret = EXIT_SUCCESS;
	if (game->nb_player == 1)
	{
		sem_destroy(&game->sem_player);
		sem_destroy(&game->sem_map);
	}
	else
	{
		if (sem_wait(&game->sem_player) < 0)
			dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
		game->nb_player--;
		if (sem_post(&game->sem_player) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		if (sem_wait(&game->sem_map) < 0)
			dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
		game->map[g_lemipc.pos_y][g_lemipc.pos_x] = ' ';
		if (sem_post(&game->sem_map) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		ret = END;
	}
	if (munmap(game, size) < 0)
		dprintf(STDERR_FILENO, "%s: munmap(): %s\n", PRG_NAME, strerror(errno));
	return (ret);
}
