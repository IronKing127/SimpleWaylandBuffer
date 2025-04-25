#ifndef	SHARED_MEMORY
#define	SHARED_MEMORY
#include <wayland-client.h>

class SharedMemory
{
public:
	SharedMemory(int size);
	void prepareWaylandObjects(wl_shm* base);
	void resize(int	size);
	
	int fileDescriptor;
	uint32_t* origin;
	int size;
	wl_shm_pool* pool;
private:
	bool createShm();
	bool sizeShm(int size);
	bool mapShm();
};

#endif
