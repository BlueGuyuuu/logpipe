#include "logpipe_api.h"

/* communication protocol :
	|'@'(1byte)|filename_len(2bytes)|file_name|file_block_len(2bytes)|file_block_data|...(other file blocks)...|\0\0\0\0|
*/

char	*__LOGPIPE_INPUT_TCP_VERSION = "0.1.0" ;

struct LogpipeInputPluginContext_AcceptedSession
{
	struct sockaddr_in	accepted_addr ;
	int			accepted_sock ;
} ;

funcOnLogpipeInputEvent OnLogpipeInputEvent_accepted_session ;
int OnLogpipeInputEvent_accepted_session( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	struct LogpipeInputPluginContext_AcceptedSession	*p_accepted_session_context = (struct LogpipeInputPluginContext_AcceptedSession *)p_context ;
	
	uint16_t						*filename_len_htons = NULL ;
	char							comm_buf[ 1 + sizeof(uint16_t) + PATH_MAX ] ;
	uint16_t						filename_len ;
	int							len ;
	
	int							nret = 0 ;
	
	len = readn( p_accepted_session_context->accepted_sock , comm_buf , 1+sizeof(uint16_t) ) ;
	if( len == -1 )
	{
		ERRORLOG( "recv comm magic and filename len failed , errno[%d]" , errno );
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else if( len == 0 )
	{
		INFOLOG( "remote socket closed on recv comm magic and filename len" );
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else
	{
		INFOLOG( "recv comm magic and filename len ok , [%d]bytes" , len )
		DEBUGHEXLOG( comm_buf , len , NULL )
	}
	
	if( comm_buf[0] != LOGPIPE_COMM_HEAD_MAGIC )
	{
		ERRORLOG( "magic[%c][%d] invalid" , comm_buf[0] , comm_buf[0] );
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	
	filename_len_htons = (uint16_t*)(comm_buf+1) ;
	filename_len = ntohs(*filename_len_htons) ;
	
	if( filename_len > PATH_MAX )
	{
		ERRORLOG( "filename length[%d] too long" , filename_len );
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	
	len = readn( p_accepted_session_context->accepted_sock , comm_buf+1+sizeof(uint16_t) , filename_len ) ;
	if( len == -1 )
	{
		ERRORLOG( "recv accepted session sock failed , errno[%d]" , errno );
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else if( len == 0 )
	{
		ERRORLOG( "remote socket closed on recv accepted session sock" );
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else
	{
		INFOLOG( "recv filename from socket ok , [%d]bytes" , len )
		DEBUGHEXLOG( comm_buf+1+sizeof(uint16_t) , len , NULL )
	}
	
	/* ������������� */
	nret = WriteAllOutputPlugins( p_env , p_logpipe_input_plugin , filename_len , comm_buf+1+sizeof(uint16_t) ) ;
	if( nret )
	{
		ERRORLOG( "WriteAllOutputPlugins failed[%d]" , nret )
		return nret;
	}
	
	return 0;
}

funcBeforeReadLogpipeInput BeforeReadLogpipeInput_accepted_session ;
int BeforeReadLogpipeInput_accepted_session( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	return 0;
}

funcReadLogpipeInput ReadLogpipeInput_accepted_session ;
int ReadLogpipeInput_accepted_session( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context , uint32_t *p_block_len , char *block_buf , int block_bufsize )
{
	struct LogpipeInputPluginContext_AcceptedSession	*p_accepted_session_context = (struct LogpipeInputPluginContext_AcceptedSession *)p_context ;
	uint32_t						block_len_htonl ;
	int							len ;
	
	len = readn( p_accepted_session_context->accepted_sock , & block_len_htonl , sizeof(uint32_t) ) ;
	if( len == -1 )
	{
		ERRORLOG( "recv block length from accepted session sock failed , errno[%d]" , errno )
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else if( len == 0 )
	{
		ERRORLOG( "accepted sessio sock closed on recv block length" )
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else
	{
		INFOLOG( "recv block length from accepted session sock ok , [%d]bytes" , sizeof(uint32_t) )
		DEBUGHEXLOG( (char*) & block_len_htonl , len , NULL )
	}
	
	(*p_block_len) = ntohl( block_len_htonl ) ;
	if( (*p_block_len) == 0 )
		return LOGPIPE_READ_END_OF_INPUT;
	if( (*p_block_len) > block_bufsize-1 )
	{
		ERRORLOG( "block length[%d] too long" , (*p_block_len) )
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	
	len = readn( p_accepted_session_context->accepted_sock , block_buf , (*p_block_len) ) ;
	if( len == -1 )
	{
		ERRORLOG( "recv block from accepted session sock failed , errno[%d]" , errno )
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else if( len == 0 )
	{
		ERRORLOG( "accepted session socket closed on recv block" )
		RemoveLogpipeInputSession( p_env , p_logpipe_input_plugin );
		return 1;
	}
	else
	{
		INFOLOG( "recv block from accepted session sock ok , [%d]bytes" , (*p_block_len) )
		DEBUGHEXLOG( block_buf , len , NULL )
	}
	
	return 0;
}

funcAfterReadLogpipeInput AfterReadLogpipeInput_accepted_session ;
int AfterReadLogpipeInput_accepted_session( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	return 0;
}

funcCleanLogpipeInputPlugin CleanLogpipeInputPlugin_accepted_session ;
int CleanLogpipeInputPlugin_accepted_session( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	struct LogpipeInputPluginContext_AcceptedSession	*p_accepted_session_context = (struct LogpipeInputPluginContext_AcceptedSession *)p_context ;
	
	if( p_accepted_session_context->accepted_sock >= 0 )
	{
		INFOLOG( "close accepted sock" )
		close( p_accepted_session_context->accepted_sock ); p_accepted_session_context->accepted_sock = -1 ;
	}
	
	INFOLOG( "free p_accepted_session_context" )
	free( p_accepted_session_context );
	
	return 0;
}

/****************** ����Ϊ�����ӻỰ ******************/

struct LogpipeInputPluginContext
{
	char			*ip ;
	int			port ;
	
	struct sockaddr_in	listen_addr ;
	int			listen_sock ;
} ;

funcInitLogpipeInputPlugin InitLogpipeInputPlugin ;
int InitLogpipeInputPlugin( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , struct LogpipePluginConfigItem *p_plugin_config_items , void **pp_context , int *p_fd )
{
	struct LogpipeInputPluginContext	*p_plugin_ctx = NULL ;
	char					*p = NULL ;
	
	int					nret = 0 ;
	
	p_plugin_ctx = (struct LogpipeInputPluginContext *)malloc( sizeof(struct LogpipeInputPluginContext) ) ;
	if( p_plugin_ctx == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( p_plugin_ctx , 0x00 , sizeof(struct LogpipeInputPluginContext) );
	
	/* ����������� */
	p_plugin_ctx->ip = QueryPluginConfigItem( p_plugin_config_items , "ip" ) ;
	INFOLOG( "ip[%s]" , p_plugin_ctx->ip )
	
	p = QueryPluginConfigItem( p_plugin_config_items , "port" ) ;
	if( p )
		p_plugin_ctx->port = atoi(p) ;
	else
		p_plugin_ctx->port = 0 ;
	INFOLOG( "port[%d]" , p_plugin_ctx->port )
	
	/* ��ʼ����������ڲ����� */
	memset( & (p_plugin_ctx->listen_addr) , 0x00 , sizeof(struct sockaddr_in) );
	p_plugin_ctx->listen_addr.sin_family = AF_INET ;
	if( p_plugin_ctx->ip[0] == '\0' )
		p_plugin_ctx->listen_addr.sin_addr.s_addr = INADDR_ANY ;
	else
		p_plugin_ctx->listen_addr.sin_addr.s_addr = inet_addr(p_plugin_ctx->ip) ;
	p_plugin_ctx->listen_addr.sin_port = htons( (unsigned short)(p_plugin_ctx->port) );
	
	/* ���������� */
	p_plugin_ctx->listen_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( p_plugin_ctx->listen_sock == -1 )
	{
		ERRORLOG( "socket failed , errno[%d]" , errno );
		return -1;
	}
	
	/* �����׽���ѡ�� */
	{
		int	onoff = 1 ;
		setsockopt( p_plugin_ctx->listen_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(int) );
	}
	
	{
		int	onoff = 1 ;
		setsockopt( p_plugin_ctx->listen_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	/* ���׽��ֵ������˿� */
	nret = bind( p_plugin_ctx->listen_sock , (struct sockaddr *) & (p_plugin_ctx->listen_addr) , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ERRORLOG( "bind[%s:%d][%d] failed , errno[%d]" , p_plugin_ctx->ip , p_plugin_ctx->port , p_plugin_ctx->listen_sock , errno );
		return -1;
	}
	
	/* ��������״̬�� */
	nret = listen( p_plugin_ctx->listen_sock , 10240 ) ;
	if( nret == -1 )
	{
		ERRORLOG( "listen[%s:%d][%d] failed , errno[%d]" , p_plugin_ctx->ip , p_plugin_ctx->port , p_plugin_ctx->listen_sock , errno );
		return -1;
	}
	else
	{
		INFOLOG( "listen[%s:%d][%d] ok" , p_plugin_ctx->ip , p_plugin_ctx->port , p_plugin_ctx->listen_sock )
	}
	
	/* ���ò������������ */
	(*pp_context) = p_plugin_ctx ;
	(*p_fd) = p_plugin_ctx->listen_sock ;
	
	return 0;
}

funcInitLogpipeInputPlugin2 InitLogpipeInputPlugin2 ;
int InitLogpipeInputPlugin2( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , struct LogpipePluginConfigItem *p_plugin_config_items , void **pp_context , int *p_fd )
{
	return 0;
}

funcOnLogpipeInputEvent OnLogpipeInputEvent ;
int OnLogpipeInputEvent( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	struct LogpipeInputPluginContext			*p_plugin_ctx = (struct LogpipeInputPluginContext *)p_context ;
	struct LogpipeInputPluginContext_AcceptedSession	*p_logpipe_input_plugin_tcp_accepted = NULL ;
	socklen_t						accept_addr_len ;
	
	/* �����ڴ��Դ�ſͻ������ӻỰ */
	p_logpipe_input_plugin_tcp_accepted = (struct LogpipeInputPluginContext_AcceptedSession *)malloc( sizeof(struct LogpipeInputPluginContext_AcceptedSession) ) ;
	if( p_logpipe_input_plugin_tcp_accepted == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno )
		return -1;
	}
	memset( p_logpipe_input_plugin_tcp_accepted , 0x00 , sizeof(struct LogpipeInputPluginContext_AcceptedSession) );
	
	/* ���������� */
	accept_addr_len = sizeof(struct sockaddr) ;
	p_logpipe_input_plugin_tcp_accepted->accepted_sock = accept( p_plugin_ctx->listen_sock , (struct sockaddr *) & (p_logpipe_input_plugin_tcp_accepted->accepted_addr) , & accept_addr_len ) ;
	if( p_logpipe_input_plugin_tcp_accepted->accepted_sock == -1 )
	{
		ERRORLOG( "accept failed , errno[%d]" , errno )
		free( p_logpipe_input_plugin_tcp_accepted );
		return -1;
	}
	
	/* �����׽���ѡ�� */
	{
		int	onoff = 1 ;
		setsockopt( p_logpipe_input_plugin_tcp_accepted->accepted_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(int) );
	}
	
	{
		int	onoff = 1 ;
		setsockopt( p_logpipe_input_plugin_tcp_accepted->accepted_sock , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) );
	}
	
	DEBUGLOG( "[%d]accept[%d] ok" , p_plugin_ctx->listen_sock , p_logpipe_input_plugin_tcp_accepted->accepted_sock )
	
	p_logpipe_input_plugin = AddLogpipeInputSession( p_env , "accepted_session" , & OnLogpipeInputEvent_accepted_session , & BeforeReadLogpipeInput_accepted_session , & ReadLogpipeInput_accepted_session , & AfterReadLogpipeInput_accepted_session , & CleanLogpipeInputPlugin_accepted_session , p_logpipe_input_plugin_tcp_accepted->accepted_sock , p_logpipe_input_plugin_tcp_accepted ) ;
	if( p_logpipe_input_plugin == NULL )
	{
		close( p_logpipe_input_plugin_tcp_accepted->accepted_sock );
		free( p_logpipe_input_plugin_tcp_accepted );
		ERRORLOG( "AddLogpipeInputSession failed" )
		return -1;
	}
	
	return 0;
}

funcBeforeReadLogpipeInput BeforeReadLogpipeInput ;
int BeforeReadLogpipeInput( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	return 0;
}

funcReadLogpipeInput ReadLogpipeInput ;
int ReadLogpipeInput( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context , uint32_t *p_block_len , char *block_buf , int block_bufsize )
{
	return 0;
}

funcAfterReadLogpipeInput AfterReadLogpipeInput ;
int AfterReadLogpipeInput( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	return 0;
}

funcCleanLogpipeInputPlugin CleanLogpipeInputPlugin ;
int CleanLogpipeInputPlugin( struct LogpipeEnv *p_env , struct LogpipeInputPlugin *p_logpipe_input_plugin , void *p_context )
{
	struct LogpipeInputPluginContext	*p_plugin_ctx = (struct LogpipeInputPluginContext *)p_context ;
	
	if( p_plugin_ctx->listen_sock >= 0 )
	{
		INFOLOG( "close listen sock" )
		close( p_plugin_ctx->listen_sock ); p_plugin_ctx->listen_sock = -1 ;
	}
	
	INFOLOG( "free p_plugin_ctx" )
	free( p_plugin_ctx );
	
	return 0;
}

