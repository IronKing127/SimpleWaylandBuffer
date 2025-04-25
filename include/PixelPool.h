#ifndef	PIXEL_POOL
#define	PIXEL_POOL

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "SharedMemory.h"


struct PixelBuffer
{
	PixelBuffer();
	PixelBuffer(int	width, int height);
	void prepareWaylandObjects(wl_shm_pool*	pool, uint32_t*	poolOrigin, int	offset);
	uint32_t* origin;
	int width;
	int height;
	int size;
	bool ready;
	wl_buffer* buffer;
};
/*
	* Since	we must	have the wl_shm	object to make a wl_pool the initialization is done in two steps
	*	1. The constructor which prepares the shared memory region
	*	2. The prepareWaylandObjects function which creates the	wayland	memory pool
	*
	*/
class PixelPool
{
public:
	//Constants
	const static wl_shm_format PIXEL_FORMAT;
	const static int PIXEL_FORMAT_SIZE;

	PixelPool();
	PixelPool(int width, int height);

	void prepareWaylandObjects(wl_shm* shmBase);
	void resize(int	width, int height);
	void attachAndSwap(wl_surface* surface);
	PixelBuffer* getCurrentBuffer();
private:
	wl_shm*	m_shmBase;
	SharedMemory m_memorySegment;
	PixelBuffer m_buffers[2];
	int m_currentBuffer;
	int m_pendingWidth;
	int m_pendingHeight;

	//Buffer Listener
	static void bufferReleaseHandler(void *data, struct wl_buffer *wl_buffer);

	constexpr static wl_buffer_listener BUFFER_LISTENER = {bufferReleaseHandler};
};

#endif
