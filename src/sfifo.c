/*
------------------------------------------------------------
	SFIFO 1.3
------------------------------------------------------------
 * Simple portable lock-free FIFO
 * (c) 2000-2002, David Olofson
-----------------------------------------------------------
TODO:
	* Is there a way to avoid losing one byte of buffer
	  space to avoid extra variables or locking?

	* Test more compilers and environments.
-----------------------------------------------------------
 */

#ifdef __KERNEL__
#	include	<linux/string.h>
#	include	<asm/uaccess.h>
#	include	<linux/malloc.h>
#	define	malloc(x)	kmalloc(x, GFP_KERNEL)
#	define	free(x, y)	kfree_s(x, y)
#else
#	include	<string.h>
#	include	<stdlib.h>
#	define	free(x, y)	free(x)
#endif

#include "sfifo.h"

#ifdef _SFIFO_TEST_
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#define DBG(x)	/*(x)*/
#define TEST_BUFSIZE	10
#else
#define DBG(x)
#endif


/*
 * Alloc buffer, init FIFO etc...
 */
int sfifo_init(sfifo_t *f, int size)
{
	memset(f, 0, sizeof(sfifo_t));

	if(size > SFIFO_MAX_BUFFER_SIZE)
		return -EINVAL;

	/*
	 * Set sufficient power-of-2 size.
	 *
	 * No, there's no bug. If you need
	 * room for N bytes, the buffer must
	 * be at least N+1 bytes. (The fifo
	 * can't tell 'empty' from 'full'
	 * without unsafe index manipulations
	 * otherwise.)
	 */
	f->size = 1;
	for(; f->size <= size; f->size <<= 1)
		;

	/* Get buffer */
	if( 0 == (f->buffer = (void *)malloc(f->size)) )
		return -ENOMEM;

	return 0;
}

/*
 * Dealloc buffer etc...
 */
void sfifo_close(sfifo_t *f)
{
	if(f->buffer)
		free(f->buffer, f->size);
}

/*
 * Empty FIFO buffer
 */
void sfifo_flush(sfifo_t *f)
{
	/* Reset positions */
	f->readpos = 0;
	f->writepos = 0;
}

/*
 * Write bytes to a FIFO
 * Return number of bytes written, or an error code
 */
int sfifo_write(sfifo_t *f, const void *_buf, int len)
{
	int total;
	int i;
	const char *buf = (const char *)_buf;

	if(!f->buffer)
		return -ENODEV;	/* No buffer! */

	/* total = len = min(space, len) */
	total = sfifo_space(f);
	DBG(fprintf(stderr,"sfifo_space() = %d\n",total));
	if(len > total)
		len = total;
	else
		total = len;

	i = f->writepos;
	if(i + len > f->size)
	{
		memcpy(f->buffer + i, buf, f->size - i);
		buf += f->size - i;
		len -= f->size - i;
		i = 0;
	}
	memcpy(f->buffer + i, buf, len);
	f->writepos = i + len;

	return total;
}

#ifdef __KERNEL__
/*
 * Write bytes to a FIFO from a user space buffer
 * Return number of bytes written, or an error code
 */
int sfifo_write_user(sfifo_t *f, const void *buf, int len)
{
	int total;
	int i;

	if(!f->buffer)
		return -ENODEV;	/* No buffer! */

	/* total = len = min(space, len) */
	total = sfifo_space(f);
	if(len > total)
		len = total;
	else
		total = len;

	i = f->writepos;
	if(i + len > f->size)
	{
		if(f->size - i)
		{
			if(copy_from_user(f->buffer + i, buf, f->size - i))
				return -EFAULT;
			buf += f->size - i;
			len -= f->size - i;
		}
		i = 0;
	}
	if(copy_from_user(f->buffer + i, buf, len))
		return -EFAULT;
	f->writepos = i + len;

	return total;
}
#endif

/*
 * Read bytes from a FIFO
 * Return number of bytes read, or an error code
 */
int sfifo_read(sfifo_t *f, void *_buf, int len)
{
	int total;
	int i;
	char *buf = (char *)_buf;

	if(!f->buffer)
		return -ENODEV;	/* No buffer! */

	/* total = len = min(used, len) */
	total = sfifo_used(f);
	DBG(fprintf(stderr,"sfifo_used() = %d\n",total));
	if(len > total)
		len = total;
	else
		total = len;

	i = f->readpos;
	if(i + len > f->size)
	{
		memcpy(buf, f->buffer + i, f->size - i);
		buf += f->size - i;
		len -= f->size - i;
		i = 0;
	}
	memcpy(buf, f->buffer + i, len);
	f->readpos = i + len;

	return total;
}

#ifdef __KERNEL__
/*
 * Read bytes from a FIFO into a user space buffer
 * Return number of bytes read, or an error code
 */
int sfifo_read_user(sfifo_t *f, void *buf, int len)
{
	int total;
	int i;

	if(!f->buffer)
		return -ENODEV;	/* No buffer! */

	/* total = len = min(used, len) */
	total = sfifo_used(f);
	if(len > total)
		len = total;
	else
		total = len;

	i = f->readpos;
	if(i + len > f->size)
	{
		if(f->size - i)
		{
			if(copy_to_user(buf, f->buffer + i, f->size - i))
				return -EFAULT;
			buf += f->size - i;
			len -= f->size - i;
		}
		i = 0;
	}
	if(copy_to_user(buf, f->buffer + i, len))
		return -EFAULT;
	f->readpos = i + len;

	return total;
}
#endif

#ifdef _SFIFO_TEST_
void *sender(void *arg)
{
	char buf[TEST_BUFSIZE*2];
	int i,j;
	int cnt = 0;
	int res;
	sfifo_t *sf = (sfifo_t *)arg;
	while(1)
	{
		j = sfifo_space(sf);
		for(i = 0; i < j; ++i)
		{
			++cnt;
			buf[i] = cnt;
		}
		res = sfifo_write(sf, &buf, j);
		if(res != j)
		{
			fprintf(stderr,"Write failed!\n");
			sleep(1);
		} else if(res)
			fprintf(stderr,"Wrote %d\n", res);
	}
}
int main()
{
	sfifo_t sf;
	char last = 0;
	pthread_t thread;
	char buf[100] = "---------------------------------------";

	fprintf(stderr,"sfifo_init(&sf, %d) = %d\n",
			TEST_BUFSIZE, sfifo_init(&sf, TEST_BUFSIZE)
		);

#if 0
	fprintf(stderr,"sfifo_write(&sf,\"0123456789\",7) = %d\n",
		sfifo_write(&sf,"0123456789",7) );

	fprintf(stderr,"sfifo_write(&sf,\"abcdefghij\",7) = %d\n",
		sfifo_write(&sf,"abcdefghij",7) );

	fprintf(stderr,"sfifo_read(&sf,buf,8) = %d\n",
		sfifo_read(&sf,buf,8) );

	buf[20] = 0;
	fprintf(stderr,"buf =\"%s\"\n",buf);

	fprintf(stderr,"sfifo_write(&sf,\"0123456789\",7) = %d\n",
		sfifo_write(&sf,"0123456789",7) );

	fprintf(stderr,"sfifo_read(&sf,buf,10) = %d\n",
		sfifo_read(&sf,buf,10) );

	buf[20] = 0;
	fprintf(stderr,"buf =\"%s\"\n",buf);
#else
	pthread_create(&thread, NULL, sender, &sf);

	while(1)
	{
		static int c = 0;
		++last;
		while( sfifo_read(&sf,buf,1) != 1 )
			/*sleep(1)*/;
		if(last != buf[0])
		{
			fprintf(stderr,"Error %d!\n", buf[0]-last);
			last = buf[0];
		}
		else
			fprintf(stderr,"Ok. (%d)\n",++c);
	}
#endif

	sfifo_close(&sf);
	fprintf(stderr,"sfifo_close(&sf)\n");

	return 0;
}

#endif
