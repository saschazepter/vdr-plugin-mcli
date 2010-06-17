#include "dvbipstream.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <mstcpip.h>
#include <Ws2ipdef.h>

#endif

#include <pthread.h>
#include <sys/types.h>

#include "clist.h"
#include "stream.h"
#include "thread.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

cStream::cStream(int Channum, in_addr_t Addr, int portnum)
		: cThread("udp streamer")
{
 	size =  188*TS_PER_UDP;
	handle = 0;
	channum = Channum;
	addr = Addr;
	m_portnum = portnum;
	
	buf = new char[size];

	if (buf == NULL)
	{
		printf("Channel: %d - Cannot allocate memory for buffer", channum);
	}

}

cStream::~cStream(void)
{
	if(buf)
		delete(buf);
}

bool cStream::StartStream(in_addr_t bindaddr)
{
	int rc=0;

	peer.sin_family = AF_INET;
	peer.sin_port = htons(m_portnum);
	peer.sin_addr.s_addr = addr;

	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_socket < 0)
	{
		log_socket_error("Stream: socket()");
		return false;
	}

#if 0
    //----------------------
    // Bind the socket. 
	sockaddr_in m_LocalAddr;

    m_LocalAddr.sin_family = AF_INET;
    m_LocalAddr.sin_port   = htons(m_portnum-1);
	m_LocalAddr.sin_addr.s_addr = bindaddr;

	rc = ::bind( udp_socket, (struct sockaddr*)&m_LocalAddr, sizeof(m_LocalAddr) );
    if ( rc < 0 )
	{
		log_socket_error("STREAM bind()");
		return false;
	}
#endif
#if 0
	char loop = 1;
	rc = setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loop, sizeof(loop));
    if ( rc < 0 )
	{
		log_socket_error("STREAM setsockopt(IP_MULTICAST_LOOP)");
		return false;
	}
#endif
	rc = setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_IF,(char *)&bindaddr, sizeof(bindaddr));
    if ( rc < 0 )
	{
		log_socket_error("STREAM setsockopt(IP_MULTICAST_IF)");
		return false;
	}
				

	pthread_mutex_lock(&lock);

	if (handle == 0)
	{
		handle = mcli_stream_setup(channum);
		if (handle)
			Start();
	}
	else
	{
		printf("cStream: handle != NULL !\n");
	}
	pthread_mutex_unlock(&lock);
	
	return true;

}

void cStream::Action()
{
	int retries = 0;
	size_t len = 0;
	off_t offset = 0;

	int     MaxFD;
    fd_set  WriteFDS;
	struct  timeval tv;

	stream_info_t *si = (stream_info_t *) handle;

	while (Running())
	{	
		pthread_mutex_lock(&si->lock_rd);

		if (si->stop)  {
			pthread_mutex_unlock(&si->lock_rd);
			break;
		}

		retries = 0;
		len = 0;
		offset = 0;

		while (retries < 50) {
//		printf("si->closed %d\n",si->closed);
			len += mcli_stream_read (handle,  buf + len, size - len, offset);
			offset += len;
			if (len == size)
				break;
			// Sleep 100ms
			usleep (100 * 1000);
			retries++;
		}
//		printf("read %s %i, offset %i\n",si->cdata->name,(int)len,(int)offset);

		int rc = 0;

		tv.tv_usec = 500000;
		tv.tv_sec = 0;
	  
		MaxFD = udp_socket;
		FD_ZERO( &WriteFDS );
		FD_SET( udp_socket, &WriteFDS );

      // wait for input
		rc = ::select( MaxFD +1, NULL, &WriteFDS, NULL, &tv );

        // log and ignore failures
        if( rc < 0 ) 
		{
            log_socket_error( "STREAM select()" );
            continue;
        }
        else if( rc > 0 ) 
		{

            // check if socket if writable
			if( FD_ISSET( udp_socket, &WriteFDS ) ) 
			{
				rc = ::sendto( udp_socket, buf, len, 0, (struct sockaddr *)&peer, sizeof(peer) );
				if (rc < 0)
				{
#ifndef WIN32
					if ( (errno == EINTR) || (errno == EWOULDBLOCK) )
						continue;
#else
				    rc = WSAGetLastError();
					if ( (rc == WSAEINTR) || (rc == WSAEWOULDBLOCK) )
						continue;
#endif
					log_socket_error("STREAM: sendto()");
					pthread_mutex_unlock(&si->lock_rd);
					break;
				}
			}
		}
		pthread_mutex_unlock(&si->lock_rd);
	}
}

void cStream::StopStream()
{
	Cancel(3);

	pthread_mutex_lock(&lock);

	if (handle)
	{
		mcli_stream_stop(handle);
		handle = 0;
	}
	else
	{
		printf("cStream: handle == NULL !\n");
	}
	pthread_mutex_unlock(&lock);

#ifdef WIN32
	closesocket(udp_socket);
#else
	close(udp_socket);
#endif

}

