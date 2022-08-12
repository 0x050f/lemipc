#include "lemipc.h"

void		recv_msg(struct ipc *ipc, char buff[256])
{
	pid_t			pid;
	int				nr;
	struct msgbuf	msg;
	struct msqid_ds	tmpbuf;

	if (msgctl(ipc->mq_id, IPC_STAT, &tmpbuf) < 0)
	{
		dprintf(STDERR_FILENO, "%s: msgctl(): %s\n", PRG_NAME, strerror(errno));
		exit(EXIT_FAILURE); // TODO: remove
	}
	printf("===================\n");
	printf("msgqnum: %d\n", tmpbuf.msg_qnum);
	pid = getpid();
	if ((nr = msgrcv(ipc->mq_id, &msg, 256, pid, 0)) >= 0)
	{
		printf("msgrcv: %s\n", msg.mtext);
		append_msg_chatbox(ipc->chatbox, msg.mtext, nr);
		if (buff)
			memcpy(buff, msg.mtext, nr);
	}
	else
	{
		dprintf(STDERR_FILENO, "%s: msgrcv(): %s\n", PRG_NAME, strerror(errno));
		exit(EXIT_FAILURE); //TODO: remove
	}
}

int			check_recv_msg(struct ipc *ipc)
{
	pid_t			pid;
	int				nr;
	struct msgbuf	msg;

	pid = getpid();
	if ((nr = msgrcv(ipc->mq_id, &msg, sizeof(msg.mtext), pid, IPC_NOWAIT)) >= 0)
	{
		append_msg_chatbox(ipc->chatbox, msg.mtext, nr);
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

void		send_msg_pid(struct ipc *ipc, pid_t pid, char *msg)
{
	int				size;
	struct msgbuf	mbuf;

	mbuf.mtype = pid;
	size = strlen(msg);
	memcpy(mbuf.mtext, msg, size);
	if (msgsnd(ipc->mq_id, &mbuf, size, 0) < 0)
	{
		dprintf(STDERR_FILENO, "%s: msgsnd(): %s\n", PRG_NAME, strerror(errno));
		exit(EXIT_FAILURE); //TODO: remove
	}
}

void		send_msg_self(struct ipc *ipc, char *msg)
{
	pid_t		pid;

	pid = getpid();
	send_msg_pid(ipc, pid, msg);
}

void		send_msg_team(struct ipc *ipc, char *msg)
{
	pid_t		pid;
	char		*msg_team;

	pid = getpid();
	msg_team = malloc(3 + strlen(msg));
	if (!(msg_team)) // TODO: error malloc
		return ;
	sprintf(msg_team, "%d: %s", ipc->player.team, msg);
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1 && ipc->game->players[i].pid != pid)
			send_msg_pid(ipc, ipc->game->players[i].pid, msg_team);
		else if (ipc->game->players[i].pid == pid)
			append_msg_chatbox(ipc->chatbox, msg_team, strlen(msg_team));
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
	free(msg_team);
}

void		send_msg_broadcast(struct ipc *ipc, char *msg)
{
	pid_t		pid;

	pid = getpid();
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1 && ipc->game->players[i].pid != pid)
			send_msg_pid(ipc, ipc->game->players[i].pid, msg);
		else if (ipc->game->players[i].pid == pid)
			append_msg_chatbox(ipc->chatbox, msg, strlen(msg));
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
}
