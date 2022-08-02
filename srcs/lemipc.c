#include "lemipc.h"

t_lemipc	g_lemipc;

size_t		align_up(size_t size, size_t align) {
	return (size + (align - (size % align)));
}

int			create_game(int fd)
{
	printf("Creating the game...\n");
	size_t	size = size = align_up(sizeof(t_game), getpagesize());
	if (ftruncate(fd, size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: ftruncate(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	/* Init mem */
	memset(addr, 0, size);
	/* Init map */
	t_game *game = addr;
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
			game->map[y][x] = '0';
	}
	return (size);
}

void		show_map(uint8_t map[HEIGHT][WIDTH])
{
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
			printf("%c", map[y][x]);
		printf("\n");
	}
}

int			join_game(int shm_fd, int mq_fd, size_t size, char *team_name)
{
	(void)team_name;
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	t_game *game = addr;
	show_map(game->map);
	while (1)
	{
		struct mq_attr attr;
		if (mq_getattr(mq_fd, &attr) < 0)
			dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
		printf("mq_flags: %ld\n", attr.mq_flags);
		printf("mq_maxmsg: %ld\n", attr.mq_maxmsg);
		printf("mq_msgsize: %ld\n", attr.mq_msgsize);
		printf("mq_cur: %ld\n", attr.mq_curmsgs);
		int nb = attr.mq_curmsgs;
		/* TEST */
		char	buffer[attr.mq_msgsize];
		int		ret;
		while (nb--)
		{
			if ((ret = mq_receive(mq_fd, buffer, sizeof(buffer), 0)) < 0)
				dprintf(STDERR_FILENO, "%s: mq_receive(): %s\n", PRG_NAME, strerror(errno));
			printf("ret: %d - msg: %.*s\n", ret,  ret, buffer);
		}
		printf("before read\n");
		ret = read(STDIN_FILENO, buffer, sizeof(buffer));
		printf("after read\n");
		if (mq_send(mq_fd, buffer, ret, 0) < 0)
			dprintf(STDERR_FILENO, "%s: mq_send(): %s\n", PRG_NAME, strerror(errno));
	}
//	sprintf(addr, "Bonjour bonjour");
//	sleep(10);
	//
	if (munmap(addr, size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: munmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

void		signal_handler(int signum)
{
	(void)signum;
	if (g_lemipc.addr)
		munmap(g_lemipc.addr, g_lemipc.size);
	if (g_lemipc.shm_fd >= 0)
		close(g_lemipc.shm_fd);
	if (g_lemipc.mq_fd >= 0)
		mq_close(g_lemipc.mq_fd);
//	TODO: only unlink if last player
//	mq_unlink("/"PRG_NAME);
//	shm_unlink(PRG_NAME);
	printf("\b\bLeaving the game...\n");
	exit(EXIT_SUCCESS);
}

int			lemipc(char *team_name)
{
	int		ret;
	struct	stat sb;

	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((g_lemipc.shm_fd = shm_open(PRG_NAME, O_CREAT | O_RDWR, 0644)) < 0)
	{
		dprintf(STDERR_FILENO, "%s: shm_open(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((g_lemipc.mq_fd = mq_open("/"PRG_NAME, O_CREAT | O_RDWR, 0644, NULL)) < 0)
	{
		dprintf(STDERR_FILENO, "%s: mq_open(): %s\n", PRG_NAME, strerror(errno));
		close(g_lemipc.shm_fd);
		return (EXIT_FAILURE);
	}
	if (fstat(g_lemipc.shm_fd, &sb) < 0)
	{
		dprintf(STDERR_FILENO, "%s: fstat(): %s\n", PRG_NAME, strerror(errno));
		ret = EXIT_FAILURE;
		goto end;
	}
	if (!sb.st_size)
	{
		if ((ret = create_game(g_lemipc.shm_fd)) == EXIT_FAILURE)
			goto end;
		ret = join_game(g_lemipc.shm_fd, g_lemipc.mq_fd, ret, team_name);
	}
	else
		ret = join_game(g_lemipc.shm_fd, g_lemipc.mq_fd, sb.st_size, team_name);
end:
	mq_close(g_lemipc.mq_fd);
	close(g_lemipc.shm_fd);
//	TODO: only unlink if last player
//	mq_unlink("/"PRG_NAME);
//	shm_unlink(PRG_NAME);
	return (ret);
}

int			main(int argc, char *argv[])
{
	if (argc != 2)
	{
		dprintf(STDERR_FILENO, "usage: ./%s team_name\n", PRG_NAME);
		return (EXIT_FAILURE);
	}
	g_lemipc.shm_fd = -1;
	g_lemipc.mq_fd = -1;
	g_lemipc.size = 0;
	g_lemipc.addr = NULL;
	return (lemipc(argv[1]));
}
