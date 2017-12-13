#include "logpipe_in.h"

static sig_atomic_t		g_QUIT_flag = 0 ;

static void sig_set_flag( int sig_no )
{
	/* ���յ���ͬ�ź����ò�ͬ��ȫ�ֱ�־���Ӻ��������д��� */
	if( sig_no == SIGTERM )
	{
		g_QUIT_flag = 1 ; /* �˳� */
	}
	
	return;
}

int monitor( struct LogpipeEnv *p_env )
{
	struct sigaction	act ;
	
	pid_t			pid , pid2 ;
	int			status ;
	
	int			nret = 0 ;
	
	/* �����ź� */
	if( ! p_env->no_daemon )
	{
		signal( SIGINT , SIG_IGN );
	}
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGPIPE , SIG_IGN );
	act.sa_handler = & sig_set_flag ;
	sigemptyset( & (act.sa_mask) );
	act.sa_flags = 0 ;
	sigaction( SIGTERM , & act , NULL );
	act.sa_flags = SA_RESTART ;
	signal( SIGCLD , SIG_DFL );
	
	while( g_QUIT_flag == 0 )
	{
		nret = pipe( p_env->quit_pipe ) ;
		if( nret == -1 )
		{
			FATALLOG( "pipe failed , errno[%d]" , errno )
			return -1;
		}
		
		/* ������������ */
		pid = fork() ;
		if( pid == -1 )
		{
			FATALLOG( "fork failed , errno[%d]" , errno )
			return -1;
		}
		else if( pid == 0 )
		{
			close( p_env->quit_pipe[1] );
			INFOLOG( "child : [%ld] fork [%ld]" , getppid() , getpid() )
			return -worker( p_env );
		}
		else
		{
			close( p_env->quit_pipe[0] );
			INFOLOG( "parent : [%ld] fork [%ld]" , getpid() , pid )
		}
		
_GOTO_WAITPID :
		
		/* �����ȴ��������̽��� */
		DEBUGLOG( "waitpid ..." )
		pid2 = waitpid( pid , & status , 0 );
		if( pid2 == -1 )
		{
			if( errno == EINTR )
			{
				/* ������˳��ź��жϣ��˳� */
				if( g_QUIT_flag )
				{
					close( p_env->quit_pipe[1] );
					goto _GOTO_WAITPID;
				}
			}
			
			ERRORLOG( "waitpid failed , errno[%d]" , errno )
			return -1;
		}
		else if( pid2 != pid )
		{
			FATALLOG( "unexpect other child[%d]" , pid2 )
		}
		
		/* ��鹤�������Ƿ��������� */
		if( WEXITSTATUS(status) == 0 && WIFSIGNALED(status) == 0 && WTERMSIG(status) == 0 )
		{
			INFOLOG( "waitpid[%d] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d]" , pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) )
		}
		else
		{
			ERRORLOG( "waitpid[%d] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d]" , pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) )
		}
		
		close( p_env->quit_pipe[1] );
		
		sleep(1);
		
		/* ���´�������ܵ��������������� */
	}
	
	/* �ر��¼����� */
	close( p_env->epoll_fd );
	
	return 0;
}

int _monitor( void *pv )
{
	struct LogpipeEnv	*p_env = (struct LogpipeEnv *)pv ;
	
	int			nret = 0 ;
	
	SetLogFile( p_env->log_file );
	SetLogLevel( p_env->log_level );
	
	nret = monitor( p_env ) ;
	
	CleanEnvironment( p_env );
	
	return nret;
}

