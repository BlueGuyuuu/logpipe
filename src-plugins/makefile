# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			logpipe-input-file.c \
			logpipe-output-file.c \
			logpipe-input-tcp.c \
			logpipe-output-tcp.c \
			logpipe-input-exec.c \

###### 目标文件、安装目录配置区
include makeinstall
LIB		=	\
			logpipe-input-file.so \
			logpipe-output-file.so \
			logpipe-input-tcp.so \
			logpipe-output-tcp.so \
			logpipe-input-exec.so \

LIBINST		=	$(_LIBINST)

###### 编译选项
CFLAGS		=	$(_CFLAGS) \
			-std=gnu99 \
			-I$(HOME)/include/logpipe \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \
			-L$(HOME)/lib \
			-llogpipe_api \
			-rdynamic \

###### 额外宏定义区
CLEAN_ADDITION	=

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_LIBINST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_LIBINST
#@ make_uninstall_DFTHDERINST
include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
logpipe-input-file.so		:	logpipe-input-file.o
	$(CC) -o $@ logpipe-input-file.o $(SOFLAGS) $(LFLAGS)

logpipe-output-file.so		:	logpipe-output-file.o
	$(CC) -o $@ logpipe-output-file.o $(SOFLAGS) $(LFLAGS)

logpipe-input-tcp.so		:	logpipe-input-tcp.o
	$(CC) -o $@ logpipe-input-tcp.o $(SOFLAGS) $(LFLAGS)

logpipe-output-tcp.so		:	logpipe-output-tcp.o
	$(CC) -o $@ logpipe-output-tcp.o $(SOFLAGS) $(LFLAGS)

logpipe-input-exec.so		:	logpipe-input-exec.o
	$(CC) -o $@ logpipe-input-exec.o $(SOFLAGS) $(LFLAGS)

