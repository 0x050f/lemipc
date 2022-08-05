#include "lemipc.h"

void		recv_msg(struct ipc *ipc, char buff[256])
{
	pid_t			pid;
	int				nr;
	struct msgbuf	msg;

	pid = getpid();
	if ((nr = msgrcv(ipc->mq_id, &msg, sizeof(msg.mtext), pid, MSG_NOERROR)) >= 0)
	{
		append_msg_chatbox(ipc->chatbox, msg.mtext, nr);
		if (buff)
			memcpy(buff, msg.mtext, nr);
	}
}

int			check_recv_msg(struct ipc *ipc)
{
	pid_t			pid;
	int				nr;
	struct msgbuf	msg;

	pid = getpid();
	if ((nr = msgrcv(ipc->mq_id, &msg, sizeof(msg.mtext), pid, MSG_NOERROR | IPC_NOWAIT)) >= 0)
	{
		append_msg_chatbox(ipc->chatbox, msg.mtext, nr);
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

void		send_msg_self(struct ipc *ipc, char *msg)
{
	struct msgbuf mbuf;
	char buf[256];
	pid_t pid;

	int size = strlen(msg);
	if (size > 256)
	{
		sprintf(buf, "Message too long (max 256 char)");
		send_msg_self(ipc, buf);
		return ;
	}
	pid = getpid();
	mbuf.mtype = pid;
	memcpy(mbuf.mtext, msg, size);
	msgsnd(ipc->mq_id, &mbuf, size, IPC_NOWAIT);
}

void		send_msg_team(struct ipc *ipc, char *msg)
{
	struct msgbuf mbuf;
	char buf[256];

	int size = strlen(msg);
	if (size - 3 > 256)
	{
		sprintf(buf, "Message too long (max 256 char)");
		send_msg_self(ipc, buf);
		return ;
	}
	sprintf(mbuf.mtext, "%d: ", ipc->player.team);
	memcpy(mbuf.mtext + strlen(mbuf.mtext), msg, size);
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1 && ipc->game->players[i].team == ipc->player.team)
		{
			mbuf.mtype = ipc->game->players[i].pid;
			msgsnd(ipc->mq_id, &mbuf, size + 3, IPC_NOWAIT);
		}
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
}

void		send_msg_broadcast(struct ipc *ipc, char *msg)
{
	struct msgbuf mbuf;
	char buf[256];

	int size = strlen(msg);
	if (size > 256)
	{
		sprintf(buf, "Message too long (max 256 char)");
		send_msg_self(ipc, buf);
		return ;
	}
	memcpy(mbuf.mtext, msg, size);
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1)
		{
			mbuf.mtype = ipc->game->players[i].pid;
			msgsnd(ipc->mq_id, &mbuf, size, IPC_NOWAIT);
		}
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
}
