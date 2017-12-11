#include "logpipe_in.h"

/* д���ݸ��ǵ��ļ� */
int WriteEntireFile( char *pathfilename , char *file_content , int file_len )
{
	FILE		*fp = NULL ;
	size_t		size ;
	
	if( file_len == -1 )
		file_len = strlen(file_content) ;
	
	fp = fopen( pathfilename , "wb" ) ;
	if( fp == NULL )
	{
		return -1;
	}
	
	size = fwrite( file_content , 1 , file_len , fp ) ;
	if( size != file_len )
	{
		fclose( fp );
		return -2;
	}
	
	fclose( fp );
	
	return 0;
}

/* ��ȡ�ļ������ݴ�ŵ���̬�����ڴ���� */
/* ע�⣺ʹ�����Ӧ�ø����ͷ��ڴ� */
char *StrdupEntireFile( char *pathfilename , int *p_file_len )
{
	char		*file_content = NULL ;
	int		file_len ;
	FILE		*fp = NULL ;
	
	int		nret = 0 ;
	
	fp = fopen( pathfilename , "rb" ) ;
	if( fp == NULL )
	{
		return NULL;
	}
	
	fseek( fp , 0 , SEEK_END );
	file_len  = ftell( fp ) ;
	fseek( fp , 0 , SEEK_SET );
	
	file_content = (char*)malloc( file_len+1 ) ;
	if( file_content == NULL )
	{
		fclose( fp );
		return NULL;
	}
	memset( file_content , 0x00 , file_len+1 );
	
	nret = fread( file_content , 1 , file_len , fp ) ;
	if( nret != file_len )
	{
		fclose( fp );
		free( file_content );
		return NULL;
	}
	
	fclose( fp );
	
	if( p_file_len )
		(*p_file_len) = file_len ;
	return file_content;
}

/* ת��Ϊ�ػ����� */
int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag )
{
	int	pid;
	
	pid = fork() ;
	switch( pid )
	{
		case -1:
			return -1;
		case 0:
			break;
		default		:
			return 0;
	}
	
	setsid();
	
	pid = fork() ;
	switch( pid )
	{
		case -1:
			return -2;
		case 0:
			break ;
		default:
			return 0;
	}
	
	if( close_flag )
	{
		close(0);
		close(1);
		close(2);
	}
	
	umask( 0 ) ;
	
	chdir( "/tmp" );
	
	ServerMain( pv );
	
	return 0;
}

/* ������������� */
int AddPluginConfigItem( struct LogpipePluginConfigItem *config , char *key , int key_len , char *value , int value_len )
{
	struct LogpipePluginConfigItem	*item = NULL ;
	
	item = (struct LogpipePluginConfigItem *)malloc( sizeof(struct LogpipePluginConfigItem) ) ;
	if( item == NULL )
		return -1;
	memset( item , 0x00 , sizeof(struct LogpipePluginConfigItem) );
	
	item->key = strndup( key , key_len ) ;
	if( item->key == NULL )
	{
		free( item );
		return -12;
	}
	
	item->value = strndup( value , value_len ) ;
	if( item->value == NULL )
	{
		free( item->key );
		free( item );
		return -13;
	}
	
	list_add_tail( & (item->this_node) , & (config->this_node) );
	
	return 0;
}

/* ��ѯ��������� */
char *QueryPluginConfigItem( struct LogpipePluginConfigItem *config , char *key )
{
	struct LogpipePluginConfigItem	*item = NULL ;
	
	list_for_each_entry( item , config , struct LogpipePluginConfigItem , this_node )
	{
		if( STRCMP( key , == , item->key ) )
			return item->value;
	}
	
	return NULL;
}

/* ɾ�����в�������� */
void RemoveAllPluginConfigItem( struct LogpipePluginConfigItem *config )
{
	struct LogpipePluginConfigItem	*item = NULL ;
	struct LogpipePluginConfigItem	*next_item = NULL ;
	
	list_for_each_entry_safe( item , next_item , config , struct LogpipePluginConfigItem , this_node )
	{
		list_del( & (item->this_node) );
		free( item );
	}
	
	return;
}

/* д�����ֽڿ鵽������ */
ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwriten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    
    while(nleft > 0)
    {
        if((nwriten = write(fd, ptr,nleft)) <= 0)
        {
            if(nwriten < 0 && errno == EINTR)
                nwriten = 0;
            else
                return -1;
        }
        
        nleft -= nwriten;
        ptr += nwriten;
    }
    
    return n;
}

/* �������ִӶ�ȡ�����ֽڿ� */
ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nread;
    char *ptr;
    ptr = vptr;
    nleft = n;
    
    while(nleft > 0)
    {
        if((nread = read(fd, ptr, nleft)) < 0)
        {
            if(errno == EINTR)
                nread = 0;
            else 
                return -1;
        }
        else if(nread == 0)
        {
            break;
    	}
        
        nleft -= nread;
        ptr += nread;
    }
    
    return (n-nleft);
}
