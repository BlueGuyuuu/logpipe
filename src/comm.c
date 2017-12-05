#include "logpipe_in.h"

/* ���������� */
int OnAcceptingSocket( struct LogPipeEnv *p_env , struct ListenSession *p_listen_session )
{
	struct AcceptedSession	*p_accepted_session = NULL ;
	socklen_t		accept_addr_len ;
	
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	/* �����ڴ��Դ�ſͻ������ӻỰ */
	p_accepted_session = (struct AcceptedSession *)malloc( sizeof(struct AcceptedSession) ) ;
	if( p_accepted_session == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno )
		return 1;
	}
	memset( p_accepted_session , 0x00 , sizeof(struct AcceptedSession) );
	
	p_accepted_session->session_type = LOGPIPE_SESSION_TYPE_ACCEPTED ;
	p_accepted_session->p_listen_session = p_listen_session ;
	
	/* ���������� */
	accept_addr_len = sizeof(struct sockaddr) ;
	p_accepted_session->accepted_sock = accept( p_listen_session->listen_sock , (struct sockaddr *) & (p_accepted_session->accepted_addr) , & accept_addr_len ) ;
	if( p_accepted_session->accepted_sock == -1 )
	{
		ERRORLOG( "accept failed , errno[%d]" , errno )
		free( p_accepted_session );
		return 1;
	}
	
	/* �����׽���ѡ�� */
	{
		int	opts ;
		opts = fcntl( p_accepted_session->accepted_sock , F_GETFL ) ;
		opts |= O_NONBLOCK ;
		fcntl( p_accepted_session->accepted_sock , F_SETFL , opts ) ;
	}
	
	{
		int	onoff = 1 ;
		setsockopt( p_accepted_session->accepted_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(int) );
	}
	
	{
		int	onoff = 1 ;
		setsockopt( p_accepted_session->accepted_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	DEBUGLOG( "[%d]accept ok[%d] session[%p]" , p_listen_session->listen_sock , p_accepted_session->accepted_sock , p_accepted_session )
	
	/* ����ͨѶ���ջ������ڻỰ�ṹ�� */
	p_accepted_session->comm_buf = (char*)malloc( LOGPIPE_COMM_BUFFER_INIT_SIZE + 1 ) ;
	if( p_accepted_session->comm_buf == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno )
		free( p_accepted_session );
		return 1;
	}
	memset( p_accepted_session->comm_buf , 0x00 , LOGPIPE_COMM_BUFFER_INIT_SIZE + 1 );
	p_accepted_session->comm_buf_size = LOGPIPE_COMM_BUFFER_INIT_SIZE + 1 ;
	p_accepted_session->comm_data_len = 0 ;
	p_accepted_session->comm_body_len = 0 ;
	
	/* �����������׽��ֵ�epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = p_accepted_session ;
	nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_accepted_session->accepted_sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl[%d] add[%d] failed , errno[%d]" , p_env->epoll_fd , p_accepted_session->accepted_sock , errno )
		close( p_accepted_session->accepted_sock );
		free( p_accepted_session );
		return 1;
	}
	else
	{
		DEBUGLOG( "epoll_ctl[%d] add[%d] ok" , p_env->epoll_fd , p_accepted_session->accepted_sock )
	}
	
	list_add_tail( & (p_accepted_session->this_node) , & (p_listen_session->accepted_session_list.this_node) );
	
	return 0;
}

/* �ر����� */
void OnClosingSocket( struct LogPipeEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	if( p_accepted_session )
	{
		DEBUGLOG( "close socket[%d] session[%p]" , p_accepted_session->accepted_sock , p_accepted_session )
		epoll_ctl( p_env->epoll_fd , EPOLL_CTL_DEL , p_accepted_session->accepted_sock , NULL );
		close( p_accepted_session->accepted_sock );
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
		
		tmp = (char*)realloc( p_accepted_session->comm_buf , p_accepted_session->comm_buf_size+LOGPIPE_COMM_BUFFER_INCREASE_SIZE ) ;
		if( tmp == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno )
			return 1;
		}
		p_accepted_session->comm_buf = tmp ;
		p_accepted_session->comm_buf_size += LOGPIPE_COMM_BUFFER_INCREASE_SIZE ;
	}
	
	/* �Ƕ����Ķ�һ�ѿͻ��������׽��� */
	DEBUGLOG( "read [%d] bytes at most ..." , p_accepted_session->comm_buf_size-1-p_accepted_session->comm_data_len )
	len = read( p_accepted_session->accepted_sock , p_accepted_session->comm_buf+p_accepted_session->comm_data_len , p_accepted_session->comm_buf_size-1-p_accepted_session->comm_data_len ) ;
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
		DEBUGHEXLOG( p_accepted_session->comm_buf+p_accepted_session->comm_data_len , len , "recv [%d]bytes" , len )
		p_accepted_session->comm_data_len += len ;
	}
	
	/* ����ѽ��յ����ݳ��ȴ��ڵ���4�ֽ� */
	while( p_accepted_session->comm_data_len >= sizeof(uint32_t) )
	{
		/* ���4�ֽ�ͨѶͷδ�����������֮ */
		if( p_accepted_session->comm_body_len == 0 )
		{
			p_accepted_session->comm_body_len = ntohl( *(uint32_t*)(p_accepted_session->comm_buf) ) ;
			DEBUGLOG( "parse comm total length [%d]bytes" , p_accepted_session->comm_body_len )
			if( p_accepted_session->comm_body_len <= 0 )
			{
				ERRORLOG( "comm_body_len[%d] invalid" , p_accepted_session->comm_body_len )
				return 1;
			}
		}
		
		/* ����Ѷ�����ͨѶ���ݴ��ڵ��ڵ�ǰͨѶͷ+ͨѶ�� */
		if( p_accepted_session->comm_data_len >= sizeof(uint32_t) + p_accepted_session->comm_body_len )
		{
			char		bak ;
			
			/* ���ַ����ضϣ�����Ӧ�ò㴦�� */
			bak = p_accepted_session->comm_buf[sizeof(uint32_t)+p_accepted_session->comm_body_len] ;
			p_accepted_session->comm_buf[sizeof(uint32_t)+p_accepted_session->comm_body_len] = '\0' ;
			DEBUGHEXLOG( p_accepted_session->comm_buf , sizeof(uint32_t)+p_accepted_session->comm_body_len , "processing [%ld]bytes" , sizeof(uint32_t)+p_accepted_session->comm_body_len )
			nret = CommToOutput( p_env , p_accepted_session ) ;
			p_accepted_session->comm_buf[sizeof(uint32_t)+p_accepted_session->comm_body_len] = bak ;
			if( nret )
			{
				ERRORLOG( "CommToOutput failed[%d]" , nret )
				return 1;
			}
			
			/* ����β��ճ�� */
			memmove( p_accepted_session->comm_buf , p_accepted_session->comm_buf+sizeof(uint32_t)+p_accepted_session->comm_body_len , p_accepted_session->comm_data_len-sizeof(uint32_t)-p_accepted_session->comm_body_len );
			p_accepted_session->comm_data_len -= sizeof(uint32_t)+p_accepted_session->comm_body_len ;
			p_accepted_session->comm_body_len = 0  ;
		}
		else
		{
			break;
		}
	}
	
	return 0;
}

