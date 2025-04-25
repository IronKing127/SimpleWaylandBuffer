#include "SharedMemory.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include "PixelPool.h"

SharedMemory::SharedMemory(int size)
{
	if(!createShm()) 
	{
		std::cout << "Failed to	create shared memory" << std::endl;
		return;
	}
	if(!sizeShm(size))
	{
		std::cout << "Failed to	size shared memory" << std::endl;
		return;
	}
	if(!mapShm())
	{
		std::cout << "Failed to	map shared memory" << std::endl;
		return;
	}
}
void SharedMemory::prepareWaylandObjects(wl_shm* base)
{
	wl_shm_create_pool(base, fileDescriptor, size);
}

void SharedMemory::resize(int size)
{
	if(!sizeShm(size))
		std::cout << "Failed to	size shared memory" << std::endl;
}

bool SharedMemory::createShm()
{
	//Allowed attempts for shared memory creation
	const static int ALLOWED_ATTEMPTS = 20;

	//Set file descriptor below 0
	fileDescriptor = -2;
	
	//String name of the shared memory region we are trying	to create with the default name
	std::string fileName = std::string("/sfmlbuffer");
	
	int attemptsTried = 0;
	//Attempt to create shared memory
	do
	{
		//Only create a	new file and set its file descriptor if	one of the same	name does not exist
		fileDescriptor = shm_open(fileName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);

		//Succeed and break out	of loop
		if(fileDescriptor > -1)
		{
			//Success now unlink files (Note this does not destroy the memory region as each region	has a reference	counter	which this does	not touch. Unlinking the name just makes it so that other processes cannot find	this region by name but	wayland	can still access it and	mmap with the file descriptor )
			shm_unlink(fileName.c_str());
			return true;	
		}

		//shm_open failed and set errno	some cases we can fix and try again others we just print the error ane return
		switch(errno)
		{
			case(EACCES):
				//Application doesn't have permission to create	shared memory which is a fatal error
				std::cout << "Must have	permission to create shared memory pool" << std::endl;
				return false;
				break;
			case(EEXIST):
				//An object with the default name already exists so tack on the	attempt	number to the end
				fileName += std::to_string(attemptsTried);
				break;
			case(EINTR):
				//The operation	was stopped by a system	interrupt this is harmless just	let it try again
				break;
			case(EINVAL):
				//The operating	system doesn't support created a shared	memory region with the provided	name not many ways to fix this at run time so fatal error
				std::cout << "Must have	permission to create shared memory pool" << std::endl;
				return false;
				break;
			case(EMFILE):
				//This means that all of the file descriptors avalible to the process are taken	(no one	knows what kind	of abomination would have to take place	to cause this but it sure is fatal)
				std::cout << "Must have	avalible file descriptors to allocate shared memory pool" << std::endl;
				return false;
				break;
			case(ENAMETOOLONG):
				//The name for the file	is too long(probably after a couple rounds of the eexist handler) knock	off some letters to shorten it and try again
				fileName.erase(fileName.size() - 20);
				break;
			case(ENFILE):
				//Something horrible has happened and the system has too many shared memory regions, fatal
				std::cout << "System has too many shared memory	regions" << std::endl;
				return false;
				break;
			case(ENOSPC):
				//The computers	memory is maxed	out and	there isn't enough space to load a shared memory region	this is	fatal
				std::cout << "No room in memory	to create shared memory	region"	<< std::endl;
				return false;
				break;
		}	
		attemptsTried++;
	} while(attemptsTried <	ALLOWED_ATTEMPTS);

	//Counter fail
	std::cout << "Failed to	create shared memory region in the given amount	of attempts" <<	std::endl;
	return false;
}
/**
 * @brief Resizes the shared memory pool with ftruncate	(Will always be	at least 1 byte)
 *
 * @return bool
 */
bool SharedMemory::sizeShm(int psize)
{
	const static int ALLOWED_ATTEMPTS = 20;
	int attemptsTried = 0;
	size = psize;
	do{
		//Ftruncate sets the size of the file returning	0 upon success and -1 upon failure
		if(ftruncate(fileDescriptor, size * PixelPool::PIXEL_FORMAT_SIZE) == 0)
		{
			return true;
		}
		switch(errno)
		{
			case(EINTR):
				//The operation	was stopped by a system	interrupt this is harmless just	let it try again
				break;
			case(EINVAL):
				//In ftruncate the things that can cause this are the size being 0 or the file not having write	permission
				if(size	<= 0)
				{
					size = 1;
				}
				else
				{
					std::cout << "Must have	write permissions to resize the	file" << std::endl;
					return false;
				}

				break;
			case(EFBIG):
				//The file is too big this is fatal
				std::cout << "File resize exceeds the maximum file size" << std::endl;
				return false;
				break;
			case(EIO):
				//IO error this	is fatal
				std::cout << "IO error when trying to resize file" << std::endl;
				return false;
				break;
			case(EBADF):
				//Somehow the file descriptor went bad this is a complicated fix so we are not going make a runtime fix
				std::cout << "File descriptor went bad"	<< std::endl;
				return false;
				break;
			case(EROFS):
				//The file descriptor resides in a read	only part of the file system this is bad
				std::cout << "File is in immutable section of the file system" << std::endl;
				return false;
				break;
		}
		attemptsTried++;
	}while(attemptsTried < ALLOWED_ATTEMPTS);

	//Counter fail
	std::cout << "Could not	create shared memory in	alloted	time" << std::endl;
	return false;
}

bool SharedMemory::mapShm()
{
	//Map the temp file into memory	for our	program	to use
	origin = static_cast<uint32_t*>(mmap(NULL, size	* PixelPool::PIXEL_FORMAT_SIZE,	PROT_READ | PROT_WRITE,	MAP_SHARED, fileDescriptor, 0));

	if(origin == MAP_FAILED) {
		//This is bad make sure	to close the file before keeling over and dieing
		//TODO make more complete
		std::cout << "Failed to	create buffer";
		close(fileDescriptor);
		return false;
	}
	return true;
}

