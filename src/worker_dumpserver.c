#include "logpipe_in.h"

#define MAX_EPOLL_EVENTS	10000

/* ����������־ */
int OnProcessLogAppender( struct LogPipeEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	return 0;
}

/* ���������� */
int OnAcceptingSocket( struct LogPipeEnv *p_env )
{
	struct AcceptedSession	*p_accepted_session = NULL ;
	socklen_t		accept_addr_len ;
	
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	/* �����ڴ��Դ�ſͻ������ӻỰ */
	p_accepted_session = (struct AcceptedSession *)malloc( sizeof(struct AcceptedSession) ) ;
	if( p_accepted_session == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno );
		return 1;
	}
	memset( p_accepted_session , 0x00 , sizeof(struct AcceptedSession) );
	
	/* ���������� */
	accept_addr_len = sizeof(struct sockaddr) ;
	p_accepted_session->client_sock = accept( p_env->role_context.dumpserver.listen_sock , (struct sockaddr *) & (p_accepted_session->client_addr) , & accept_addr_len ) ;
	if( p_accepted_session->client_sock == -1 )
	{
		ERRORLOG( "accept failed , errno[%d]" , errno );
		free( p_accepted_session );
		return 1;
	}
	
	{
		int	opts ;
		opts = fcntl( p_accepted_session->client_sock , F_GETFL ) ;
		opts |= O_NONBLOCK ;
		fcntl( p_accepted_session->client_sock , F_SETFL , opts ) ;
	}
	
	{
		int	onoff = 1 ;
		setsockopt( p_accepted_session->client_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(int) );
	}
	
	{
		int	onoff = 1 ;
		setsockopt( p_accepted_session->client_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	DEBUGLOG( "[%d]accept ok[%d] session[%p]" , p_env->role_context.dumpserver.listen_sock , p_accepted_session->client_sock , p_accepted_session );
	
	/* ����ͨѶ���ջ������ڻỰ�ṹ�� */
	p_accepted_session->comm_buf = (char*)malloc( COMMBUFFER_INIT_SIZE + 1 ) ;
	if( p_accepted_session->comm_buf == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno );
		free( p_accepted_session );
		return 1;
	}
	memset( p_accepted_session->comm_buf , 0x00 , COMMBUFFER_INIT_SIZE + 1 );
	p_accepted_session->comm_buf_size = COMMBUFFER_INIT_SIZE + 1 ;
	p_accepted_session->comm_data_len = 0 ;
	p_accepted_session->comm_body_len = 0 ;
	
	/* �����������׽��ֵ�epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = p_accepted_session ;
	nret = epoll_ctl( p_env->role_context.dumpserver.epoll_fd , EPOLL_CTL_ADD , p_accepted_session->client_sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl[%d] add[%d] failed , errno[%d]" , p_env->role_context.dumpserver.epoll_fd , p_accepted_session->client_sock , errno );
		close( p_accepted_session->client_sock );
		free( p_accepted_session );
		return 1;
	}
	else
	{
		DEBUGLOG( "epoll_ctl[%d] add[%d] ok" , p_env->role_context.dumpserver.epoll_fd , p_accepted_session->client_sock );
	}
	
	list_add_tail( & (p_accepted_session->this_node) , & (p_env->role_context.dumpserver.accepted_session_list.this_node) );
	
	return 0;
}

/* �ر����� */
void OnClosingSocket( struct LogPipeEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	if( p_accepted_session )
	{
		DEBUGLOG( "close socket[%d] session[%p]" , p_accepted_session->client_sock , p_accepted_session );
		epoll_ctl( p_env->role_context.dumpserver.epoll_fd , EPOLL_CTL_DEL , p_accepted_session->client_sock , NULL );
		close( p_accepted_session->client_sock );
		list_del( & (p_accepted_session->this_node) );
		free( p_accepted_session->comm_buf );
		free( p_accepted_session );
	}
	
	return;
}

/* ͨѶ�������� */
int OnReceivingSocket( struct LogPipeEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	int			len ;
	
	int			nret = 0 ;
	
	/* ���ͨѶ���ջ��������ˣ����Ÿû����� */
	if( p_accepted_session->comm_data_len == p_accepted_session->comm_buf_size-1 )
	{
		char	*tmp = NULL ;
		
		tmp = (char*)realloc( p_accepted_session->comm_buf , p_accepted_session->comm_buf_size+COMMBUFFER_INCREASE_SIZE ) ;
		if( tmp == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		p_accepted_session->comm_buf = tmp ;
		p_accepted_session->comm_buf_size += COMMBUFFER_INCREASE_SIZE ;
	}
	
	/* �Ƕ����Ķ�һ�ѿͻ��������׽��� */
	DEBUGLOG( "read [%d] bytes at most ..." , p_accepted_session->comm_buf_size-1-p_accepted_session->comm_data_len );
	len = read( p_accepted_session->client_sock , p_accepted_session->comm_buf+p_accepted_session->comm_data_len , p_accepted_session->comm_buf_size-1-p_accepted_session->comm_data_len ) ;
	if( len == -1 )
	{
		ERRORLOG( "recv failed[%d] , errno[%d]" , len , errno );
		return 1;
	}
	else if( len == 0 )
	{
		INFOLOG( "remote socket closed" );
		return 1;
	}
	else
	{
		DEBUGHEXLOG( p_accepted_session->comm_buf+p_accepted_session->comm_data_len , len , "recv [%d]bytes" , len );
		p_accepted_session->comm_data_len += len ;
	}
	
	/* ����ѽ��յ����ݳ��ȴ��ڵ���4�ֽ� */
	while( p_accepted_session->comm_data_len >= 4 )
	{
		/* ���4�ֽ�ͨѶͷδ�����������֮ */
		if( p_accepted_session->comm_body_len == 0 )
		{
			p_accepted_session->comm_body_len = ntohl( *(uint32_t*)(p_accepted_session->comm_buf) ) ;
			if( p_accepted_session->comm_body_len <= 0 )
			{
				ERRORLOG( "comm_body_len[%d] invalid" , p_accepted_session->comm_body_len );
				return 1;
			}
		}
		
		/* ����Ѷ�����ͨѶ���ݴ��ڵ��ڵ�ǰͨѶͷ+ͨѶ�� */
		if( p_accepted_session->comm_data_len >= 4+p_accepted_session->comm_body_len )
		{
			char		bak ;
			
			/* ���ַ����ضϣ�����Ӧ�ò㴦�� */
			bak = p_accepted_session->comm_buf[4+p_accepted_session->comm_body_len] ;
			p_accepted_session->comm_buf[4+p_accepted_session->comm_body_len] = '\0' ;
			DEBUGHEXLOG( p_accepted_session->comm_buf , 4+p_accepted_session->comm_body_len , "processing [%ld]bytes" , 4+p_accepted_session->comm_body_len );
			nret = OnProcessLogAppender( p_env , p_accepted_session ) ;
			p_accepted_session->comm_buf[4+p_accepted_session->comm_body_len] = bak ;
			if( nret )
			{
				ERRORLOG( "OnProcessLogAppender failed[%d] , errno[%d]" , nret , errno );
				return 1;
			}
			
			/* ����β��ճ�� */
			memmove( p_accepted_session->comm_buf , p_accepted_session->comm_buf+4+p_accepted_session->comm_body_len , p_accepted_session->comm_data_len-4-p_accepted_session->comm_body_len );
			p_accepted_session->comm_data_len -= 4+p_accepted_session->comm_body_len ;
			p_accepted_session->comm_body_len = 0  ;
		}
		else
		{
			break;
		}
	}
	
	return 0;
}

int worker_dumpserver( struct LogPipeEnv *p_env )
{
	struct epoll_event	events[ MAX_EPOLL_EVENTS ] ;
	int			epoll_nfds ;
	int			i ;
	struct epoll_event	*p_event = NULL ;
	
	struct AcceptedSession	*p_accepted_session = NULL ;
	
	int			nret = 0 ;
	
	while(1)
	{
		/* �ȴ�epoll�¼�������1�볬ʱ */
		INFOLOG( "epoll_wait[%d] ..." , p_env->role_context.dumpserver.epoll_fd );
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->role_context.dumpserver.epoll_fd , events , MAX_EPOLL_EVENTS , 1000 ) ;
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				INFOLOG( "epoll_wait[%d] interrupted" , p_env->role_context.dumpserver.epoll_fd )
			}
			else
			{
				ERRORLOG( "epoll_wait[%d] failed , errno[%d]" , p_env->role_context.dumpserver.epoll_fd , errno )
				return -1;
			}
		}
		else
		{
			INFOLOG( "epoll_wait[%d] return[%d]events" , p_env->role_context.dumpserver.epoll_fd , epoll_nfds );
		}
		
		for( i = 0 , p_event = events ; i < epoll_nfds ; i++ , p_event++ )
		{
			/* �����׽����¼� */
			if( p_event->data.ptr == p_env )
			{
				/* �ɶ��¼� */
				if( p_event->events & EPOLLIN )
				{
					nret = OnAcceptingSocket( p_env ) ;
					if( nret < 0 )
					{
						FATALLOG( "OnAcceptingSocket failed[%d]" , nret )
						return -1;
					}
					else if( nret > 0 )
					{
						INFOLOG( "OnAcceptingSocket return[%d]" , nret )
					}
					else
					{
						DEBUGLOG( "OnAcceptingSocket ok" )
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FATALLOG( "listen session err or hup event[0x%X]" , p_event->events )
					return -1;
				}
				/* �����¼� */
				else
				{
					FATALLOG( "Unknow listen session event[0x%X]" , p_event->events )
					return -1;
				}
			}
			/* �����¼������ͻ������ӻỰ�¼� */
			else
			{
				p_accepted_session = (struct AcceptedSession *)(p_event->data.ptr) ;
				
				/* �ɶ��¼� */
				if( p_event->events & EPOLLIN )
				{
					nret = OnReceivingSocket( p_env , p_accepted_session ) ;
					if( nret < 0 )
					{
						FATALLOG( "OnReceivingSocket failed[%d]" , nret )
						return -1;
					}
					else if( nret > 0 )
					{
						INFOLOG( "OnReceivingSocket return[%d]" , nret )
						OnClosingSocket( p_env , p_accepted_session );
					}
					else
					{
						DEBUGLOG( "OnReceivingSocket ok" )
					}
				}
				/* ��д�¼� */
				else if( p_event->events & EPOLLOUT )
				{
					FATALLOG( "unexpect accepted session event[0x%X]" , p_event->events )
					OnClosingSocket( p_env , p_accepted_session );
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FATALLOG( "accepted session err or hup event[0x%X]" , p_event->events )
					OnClosingSocket( p_env , p_accepted_session );
				}
				/* �����¼� */
				else
				{
					FATALLOG( "Unknow accepted session event[0x%X]" , p_event->events )
					return -1;
				}
			}
		}
	}
	
	return 0;
}

