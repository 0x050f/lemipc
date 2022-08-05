#include "lemipc.h"

void		show_map(struct game *game, struct player *player)
{
	printf("\033[2J\033[H"); /* clear screen + top */
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
			if (x == (size_t)player->pos_x && y == (size_t)player->pos_y)
				printf("\e[31m");
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

void		show_chatbox(uint8_t *chatbox)
{
	for (size_t y = 0; y <= CHAT_HEIGHT; y++)
	{
		for (size_t x = 0; x <= CHAT_WIDTH; x++)
		{
			if (x == 0 && y == 0)
				printf("┌");
			else if (x == CHAT_WIDTH && y == 0)
				printf("┐");
			else if (x == 0 && y == CHAT_HEIGHT)
				printf("└");
			else if (x == CHAT_WIDTH && y == CHAT_HEIGHT)
				printf("┘");
			else if (x == 0 || x == CHAT_WIDTH)
				printf("│");
			else if (y == 0 || y == CHAT_HEIGHT)
				printf("─");
			else
				printf("%c", chatbox[(x - 1) + (y - 1) * (CHAT_WIDTH - 1)]);
		}
		printf("\n");
	}
}

void		append_msg_chatbox(uint8_t *chatbox, char *msg, size_t size)
{
	static size_t		cursor = 0;

	size_t cursor_max_size = (CHAT_HEIGHT - 1) * (CHAT_WIDTH - 1);
	size_t size_to_add = (CHAT_WIDTH - 1) - (size % (CHAT_WIDTH - 1));
	size_t total_size = size + size_to_add;

	if (cursor + total_size > cursor_max_size)
	{
		memmove(chatbox, chatbox + total_size, (cursor_max_size - total_size));
		cursor = (cursor_max_size - total_size);
	}
	memcpy(chatbox + cursor, msg, size);
	cursor += size;
	memset(chatbox + cursor, ' ', size_to_add);
	cursor += size_to_add;
}

void		show_game(struct ipc *ipc)
{
	sem_lock(ipc->sem_id[MAP]);
	show_map(ipc->game, &ipc->player);
	sem_unlock(ipc->sem_id[MAP]);
	if (ipc->chatbox)
		show_chatbox(ipc->chatbox);
}
