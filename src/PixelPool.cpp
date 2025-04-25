#include "PixelPool.h"
#include <iostream>

//This format completely supports SFML'S color space and is required to	be supported on	every wayland compositor
const wl_shm_format PixelPool::PIXEL_FORMAT = WL_SHM_FORMAT_ARGB8888;

//There	is currently no	way to programmatically	get the	size in	bytes of the formats so	its up here with the pixel format so you don't have to chase random constants around the document (See the declaration of wl_shm_format	to see the size	of other formats)
const int PixelPool::PIXEL_FORMAT_SIZE = 4;

PixelPool::PixelPool() : m_memorySegment(600 * 600 * 2), 
	m_buffers{PixelBuffer(600, 600), PixelBuffer(600, 600)}, 
	m_currentBuffer(0), 
	m_pendingWidth(0), m_pendingHeight(0)
{

}

PixelPool::PixelPool(int width,	int height) : m_memorySegment(width * height * 2), 
	m_buffers{PixelBuffer(width, height), PixelBuffer(width, height)}, 
	m_currentBuffer(0), 
	m_pendingWidth(0), m_pendingHeight(0)
{

}

void PixelPool::prepareWaylandObjects(wl_shm* base)
{
	m_shmBase = base;
	m_memorySegment.prepareWaylandObjects(m_shmBase);
	m_buffers[0].prepareWaylandObjects(m_memorySegment.pool, m_memorySegment.origin, 0);
	m_buffers[1].prepareWaylandObjects(m_memorySegment.pool, m_memorySegment.origin, m_buffers[0].size);
}

void PixelPool::resize(int width, int height)
{
	int newBufferSize = width * height;

	//If the current buffer	isn't free there is nothing we can do now so set the pending size so that the
	//buffer release handler will call back	to us when it is ready
	if(!m_buffers[m_currentBuffer].ready)
	{
		m_pendingWidth = width;
		m_pendingHeight	= height;
		return;
	}

	//If the new size is smaller than the current size then	just resize the	buffer since it	is guarenteed to
	//fit in its previous spot otherwise special precautions need to be taken since	the buffer that	isn't the
	//current buffer is attached to	the surface and	cannot be modified
	if(newBufferSize <= m_buffers[m_currentBuffer].size)
	{
		wl_buffer_destroy(m_buffers[m_currentBuffer].buffer);
		m_buffers[m_currentBuffer] = PixelBuffer(width,	height);
		m_buffers[m_currentBuffer].prepareWaylandObjects(m_memorySegment.pool, m_memorySegment.origin,
								 (m_currentBuffer == 0)	? 0 : m_buffers[0].size);
	}
	else
	{
		//Since	the size is larger than	the previous one we might have to resize the memory pool to make
		//sure it is large enough
		if(newBufferSize * 2 > m_memorySegment.size)
		{
			m_memorySegment.resize(newBufferSize * 2);
		}

		//If the current buffer	is 0 we	cannot resize without damaging the other buffer	which is attached
		//to the server. Instead we set	the pending size so that the buffer release handler will come back
		//to use the next frame	when the current buffer	will be	1
		if(m_currentBuffer == 0)
		{
			m_pendingWidth = width;
			m_pendingHeight	= height;
		}
		else
		{
			wl_buffer_destroy(m_buffers[m_currentBuffer].buffer);
			m_buffers[m_currentBuffer] = PixelBuffer(width,	height);
			m_buffers[m_currentBuffer].prepareWaylandObjects(m_memorySegment.pool, m_memorySegment.origin, newBufferSize);
		}
	}

	//Before returning check the other buffer and set the pending size if it is not up to date so that the
	//buffer release handler can come back to us for that one
	if(!(m_buffers[m_currentBuffer ^ 1].width == width && m_buffers[m_currentBuffer ^ 1].height == height))
	{
    		m_pendingWidth = width;
    		m_pendingHeight = height;
	}
}

void PixelPool::attachAndSwap(wl_surface* surface)
{
	//Before sending off to the compositor tag on the listener so it can get back to us
	wl_buffer_add_listener(m_buffers[m_currentBuffer].buffer, &BUFFER_LISTENER, this);
	//Attach and mark as unready 
	wl_surface_attach(surface, m_buffers[m_currentBuffer].buffer, 0, 0);
	m_buffers[m_currentBuffer].ready = false;
	//Toggle the current buffer
	m_currentBuffer	^= 1;
}

void PixelPool::bufferReleaseHandler(void *data, wl_buffer *wl_buffer)
{
	PixelPool* target = static_cast<PixelPool*>(data);

	for(int	i = 0; i < 2; i++)
	{
		if(target->m_buffers[i].buffer == wl_buffer)
		{
    			//Label the buffer as ready now
			target->m_buffers[i].ready = true;
			//If there is a pending size pass the pending size into the resize function
			if(target->m_pendingHeight * target->m_pendingHeight !=	0)
			{
				target->resize(target->m_pendingHeight,	target->m_pendingHeight);
			}
			return;
		}
	}
	std::cout << "Error: Buffer from buffer	release	handler	did not	match either buffer" <<	std::endl;
}

PixelBuffer* PixelPool::getCurrentBuffer()
{
    if(m_buffers[m_currentBuffer].ready)
	return &m_buffers[m_currentBuffer];
    else
        return nullptr;
}

PixelBuffer::PixelBuffer()
{
	buffer = nullptr;
	origin = nullptr;
	width =	0;
	height = 0;
	size = 0;
	ready =	false;
}

PixelBuffer::PixelBuffer(int width, int	height)
{
	buffer = nullptr;
	origin = nullptr;
	this->width = width;
	this->height = height;
	this->size = width * height;
	ready =	true;
}

void PixelBuffer::prepareWaylandObjects(wl_shm_pool* pool, uint32_t* poolOrigin, int offset)
{
	buffer = wl_shm_pool_create_buffer(pool, PixelPool::PIXEL_FORMAT_SIZE, width, height, width * PixelPool::PIXEL_FORMAT_SIZE, PixelPool::PIXEL_FORMAT);
	origin = poolOrigin + offset;
}

