#ifndef _H_LOGPIPE_
#define _H_LOGPIPE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <limits.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

int asprintf(char **strp, const char *fmt, ...);

#include "list.h"
#include "rbtree.h"
#include "LOGC.h"

#include "IDL_logpipe_conf.dsc.h"

#define LOGPIPE_IO_TYPE_FILE		"file://"
#define LOGPIPE_IO_TYPE_TCP		"tcp://"

#define LOGPIPE_SESSION_TYPE_INOTIFY	'M'
#define LOGPIPE_SESSION_TYPE_LISTEN	'L'
#define LOGPIPE_SESSION_TYPE_ACCEPTED	'A'
#define LOGPIPE_SESSION_TYPE_DUMP	'D'
#define LOGPIPE_SESSION_TYPE_FORWARD	'F'

#define LOGPIPE_ROLE_COLLECTOR		'C'
#define LOGPIPE_ROLE_PIPER		'P'
#define LOGPIPE_ROLE_DUMPSERVER		'S'

#define LOGPIPE_INOTIFY_READ_BUFSIZE	16*1024*1024

/* communication protocol :
	|'@'(1byte)|filename_len(2bytes)|file_name|file_block_len(2bytes)|file_block_data|...(other file blocks)...|\0\0\0\0|
*/
#define LOGPIPE_COMM_MAGIC		'@'
#define LOGPIPE_COMM_FILE_BLOCK		40960

/* �Ự�ṹͷ */
struct Session
{
	unsigned char		session_type ;
} ;

/* �����ļ��Ự�ṹ */
struct TraceFile
{
	char			*path_filename ;
	uint16_t		path_filename_len ;
	char			*pathname ;
	char			*filename ;
	uint16_t		filename_len ;
	off_t			trace_offset ;
	
	int			inotify_file_wd ;
	struct rb_node		inotify_file_wd_rbnode ;
} ;

/* Ŀ¼��ض˻Ự�ṹ */
struct InotifySession
{
	unsigned char		session_type ;
	
	char			inotify_path[ PATH_MAX + 1 ] ;
	int			inotify_fd ;
	int			inotify_path_wd ;
	struct rb_root		inotify_wd_rbtree ;
	
	struct list_head	this_node ;
} ;

/* �ͻ��˻Ự�ṹ */
struct AcceptedSession
{
	unsigned char		session_type ;
	
	struct ListenSession	*p_listen_session ;
	
	struct sockaddr_in      accepted_addr ;
	int			accepted_sock ;
	
	struct list_head	this_node ;
} ;

/* �����˻Ự�ṹ */
struct ListenSession
{
	unsigned char		session_type ;
	
	char			listen_ip[ 20 + 1 ] ;
	int			listen_port ;
	
	struct sockaddr_in    	listen_addr ;
	int			listen_sock ;
	
	struct AcceptedSession	accepted_session_list ; /* �ͻ��������ӻỰ���� */
	
	struct list_head	this_node ;
} ;

/* �鼯��ض˻Ự�ṹ */
struct DumpSession
{
	unsigned char		session_type ;
	
	char			dump_path[ PATH_MAX + 1 ] ;
	
	int			tmp_fd ;
	
	struct list_head	this_node ;
} ;

/* ת���˻Ự�ṹ */
struct ForwardSession
{
	unsigned char		session_type ;
	
	char			forward_ip[ 20 + 1 ] ;
	int			forward_port ;
	
	struct sockaddr_in    	forward_addr ;
	int			forward_sock ;
	
	struct list_head	this_node ;
} ;

/* �����ṹ */
struct LogPipeEnv
{
	char			config_path_filename[ PATH_MAX + 1 ] ;
	int			no_daemon ;
	
	logpipe_conf		conf ;
	int			log_level ;
	
	int			epoll_fd ;
	
	int			is_monitor ;
	
	struct InotifySession	inotify_session_list ; /* Ŀ¼��ض˻Ự���� */
	struct ListenSession	listen_session_list ; /* �����˻Ự���� */
	
	struct DumpSession	dump_session_list ; /* �鼯��ض˻Ự���� */
	struct ForwardSession	forward_session_list ; /* ת���˻Ự���� */
	
	char			*inotify_read_buffer ;
	int			inotify_read_bufsize ;
} ;

int LinkTraceFileWdTreeNode( struct InotifySession *p_inotify_session , struct TraceFile *p_trace_file );
struct TraceFile *QueryTraceFileWdTreeNode( struct InotifySession *p_inotify_session , struct TraceFile *p_trace_file );
void UnlinkTraceFileWdTreeNode( struct InotifySession *p_inotify_session , struct TraceFile *p_trace_file );
void DestroyTraceFileTree( struct InotifySession *p_inotify_session );

int WriteEntireFile( char *pathfilename , char *file_content , int file_len );
char *StrdupEntireFile( char *pathfilename , int *p_file_len );
int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag );
ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);

void InitConfig();
int LoadConfig( struct LogPipeEnv *p_env );

int InitEnvironment( struct LogPipeEnv *p_env );
void CleanEnvironment( struct LogPipeEnv *p_env );
void LogEnvironment( struct LogPipeEnv *p_env );

int monitor( struct LogPipeEnv *p_env );
int _monitor( void *pv );

int worker( struct LogPipeEnv *p_env );

int AddFileWatcher( struct LogPipeEnv *p_env , struct InotifySession *p_inotify_session , char *filename );
int RemoveFileWatcher( struct LogPipeEnv *p_env , struct InotifySession *p_inotify_session , struct TraceFile *p_trace_file );
int RoratingFile( char *pathname , char *filename , int filename_len );
int OnReadingFile( struct LogPipeEnv *p_env , struct InotifySession *p_inotify_session , struct TraceFile *p_trace_file );
int OnInotifyHandler( struct LogPipeEnv *p_env , struct InotifySession *p_inotify_session );

int OnAcceptingSocket( struct LogPipeEnv *p_env , struct ListenSession *p_listen_session );
void OnClosingSocket( struct LogPipeEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnReceivingSocket( struct LogPipeEnv *p_env , struct AcceptedSession *p_accepted_session );

int ConnectForwardSocket( struct LogPipeEnv *p_env , struct ForwardSession *p_forward_session );
int ToOutputs( struct LogPipeEnv *p_env , char *comm_buffer , int comm_buffer_len , char *filename , uint16_t filename_len , int in , int append_len );

#ifdef __cplusplus
}
#endif

#endif

