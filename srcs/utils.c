#include "lemipc.h"

size_t		align_up(size_t size, size_t align)
{
	return (size + (align - (size % align)));
}
