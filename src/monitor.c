#include "logpipe_in.h"

static sig_atomic_t		g_SIGTERM_flag = 0 ;

static void sig_set_flag( int sig_no )
{
	/* ���յ���ͬ�ź����ò�ͬ��ȫ�ֱ�־���Ӻ��������д��� */
	if( sig_no == SIGTERM )
	{
		g_SIGTERM_flag = 1 ; /* �˳� */
	}
	
	return;
}

int monitor( struct LogPipeEnv *p_env )
{
	struct sigaction	act ;
	
	pid_t			pid ;
	int			status ;
	
	SetLogFile( p_env->conf.log.log_file );
	SetLogLevel( p_env->log_level );
	
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
	
	while(1)
	{
		/* ������������ */
		p_env->worker_pid = fork() ;
		if( p_env->worker_pid == -1 )
		{
			ERRORLOG( "fork failed , errno[%d]" , errno )
			return -1;
		}
		else if( p_env->worker_pid == 0 )
		{
			INFOLOG( "child : [%ld] fork [%ld]" , getppid() , getpid() )
			return -worker( p_env );
		}
		else
		{
			INFOLOG( "parent : [%ld] fork [%ld]" , getpid() , p_env->worker_pid )
		}
		
_GOTO_WAITPID :
		
		/* �����ȴ��������̽��� */
		DEBUGLOG( "waitpid ..." )
		pid = waitpid( p_env->worker_pid , & status , 0 );
		if( pid == -1 )
		{
			if( errno == EINTR )
			{
				/* ������˳��ź��жϣ��˳� */
				if( g_SIGTERM_flag )
				{
					break;
				}
				else
				{
					goto _GOTO_WAITPID;
				}
			}
			
			ERRORLOG( "waitpid failed , errno[%d]" , errno )
			return -1;
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
		
		sleep(1);
		
		/* ���´�������ܵ��������������� */
	}
	
	/* ɱ���ӽ��� */
	kill( p_env->worker_pid , SIGTERM );
	
	/* �ر��¼����� */
	close( p_env->epoll_fd );
	
	return 0;
}

int _monitor( void *pv )
{
	return monitor( (struct LogPipeEnv*)pv );
}

