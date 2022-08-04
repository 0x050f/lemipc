#include <lemipc.h>

int		create_game(struct ipc *ipc)
{
	ipc->game = shmat(ipc->shm_id, NULL, 0);

	memset(ipc->game, ' ', sizeof(ipc->game));
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	(void)ipc;
	return (EXIT_SUCCESS);
}
