��־�ɼ�����(logpipe)
====================
<!-- TOC -->

- [1. ����](#1-����)
- [2. ��װ](#2-��װ)
    - [2.1. Դ����밲װ](#21-Դ����밲װ)
        - [2.1.1. ���밲װlogpipe](#211-���밲װlogpipe)
        - [2.1.2. ���밲װ�Դ�logpipe���](#212-���밲װ�Դ�logpipe���)
        - [2.1.3. ȷ�ϰ�װ](#213-ȷ�ϰ�װ)
- [3. ʹ��](#3-ʹ��)
- [4. �������](#4-�������)
- [5. ����ѹ��](#5-����ѹ��)
- [6. ���](#6-���)

<!-- /TOC -->
# 1. ����

�ڼ�Ⱥ���������־�ɼ�����Ҫ������ʩ��

��Դ������������ǻ���flume-ng������ʵ��ʹ���з���flume-ng����������⣬����flume-ng��spoolDir�ɼ���ֻ�ܶ��ļ���ת����Ĵ�С���ܱ仯��������־�ļ����вɼ�����������ɼ�ʱЧ��Ҫ�����Ҫ�ɼ����ڱ�����׷�ӵ���־�ļ���ֻ����exec�ɼ�������tail -F�����tail -F�����ֲ���ͨ��Ŀ��Ŀ¼�н���������δ֪�ļ������������������logstash������JAVA�������ڴ�ռ�ú����ܶ����ܴﵽ���š�

��Ϊһ����־�ɼ��ı��ش����ڴ�ռ��Ӧ��С���ܿأ�����Ӧ�ø�Ч���ķ�CPU�Ͷ�Ӧ��Ӱ�쾡����С��Ҫ���첽ʵʱ׷����־�ļ�������ĳЩӦ�û���Ŀ��Ŀ¼�²��������־�ļ��������ڲ���ȷ����������־�ļ������ܹ���Ҫ֧�ֶ�����������ʽ��־�ɼ����䣬Ϊ�˴�������������о������輼��������ʵ���ѶȲ����ߣ���������logpipe��

logpipe��һ���ֲ�ʽ���߿��õ����ڲɼ������䡢�Խ���ص���־���ߣ������˲�����Ŀ�ܽṹ��ƣ�֧�ֶ��������������������������ʽ��־�ռ��ܹ���

![logpipe.png](logpipe.png)

logpipe������ʵ��ʹ�÷��㡢���ü�����û����sink��һ��������ʡ�

logpipe�����ɸ�input���¼����ߺ����ɸ�output��ɡ�����logpipe�������(monitor)������һ����������(worker)����ع������̱����������������̡���������װ�����ü������ɸ�input��������ɸ�output����������¼�ѭ������һinput���������Ϣ�����������output�����

logpipe�Դ���4���������󽫿��������������ֱ��ǣ�
* logpipe-input-file ��inotify�첽ʵʱ�����־Ŀ¼��һ�����ļ��½����ļ����������������ļ����Ͷ�ȡ�ļ�׷�����ݡ�ӵ���ļ���Сת�����ܣ��������Ӧ����־���Ӧ���ܣ����Ӧ����־��д��־���ܡ�֧������ѹ����
* logpipe-output-file һ������������Ϣ����������ͬ���ļ�������ļ����ݡ�֧�����ݽ�ѹ��
* logpipe-input-tcp ����TCP���������ˣ����տͻ������ӣ�һ���ͻ���������������Ϣ������������ȡ��
* logpipe-output-tcp ����TCP�ͻ��ˣ����ӷ���ˣ�һ������������Ϣ����������������ӡ�

ʹ���߿ɸ����������󣬰��ղ�������淶���������Ʋ������IBMMQ��������HDFS�������ȡ�

logpipe���ò���JSON��ʽ����η�������д��࣬��ʾ����

```
{
	"log" : 
	{
		"log_file" : "/tmp/logpipe_case1_collector.log" ,
		"log_level" : "INFO"
	} ,
	
	"inputs" : 
	[
		{ "plugin":"so/logpipe-input-file.so" , "path":"/home/calvin/log" , "compress_algorithm":"deflate" }
	] ,
	
	"outputs" : 
	[
		{ "plugin":"so/logpipe-output-tcp.so" , "ip":"127.0.0.1" , "port":10101 }
	]
}

```

# 2. ��װ

## 2.1. Դ����밲װ

### 2.1.1. ���밲װlogpipe

��[��Դ�й�](https://gitee.com/calvinwilliams/logpipe)��[github](https://github.com/calvinwilliams/logpipe)��¡����������Դ������ŵ����Դ�����Ŀ¼�н⿪�����¼�����Ĳ���ϵͳ��Linux��

����`src`Ŀ¼������õ���ִ�г���`logpipe`�Ͷ�̬��`liblogpipe_api.so`��

```
$ cd src
$ make -f makefile.Linux
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c list.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c rbtree.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c fasterjson.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c LOGC.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c config.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c env.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c util.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c output.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o liblogpipe_api.so list.o rbtree.o fasterjson.o LOGC.o config.o env.o util.o output.o -shared -L. -L/home/calvin/lib -rdynamic -ldl -lz 
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c main.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c monitor.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99  -c worker.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o logpipe main.o monitor.o worker.o -L. -L/home/calvin/lib -rdynamic -ldl -lz  -llogpipe_api
```

��ִ�г���logpipe������־�ɼ����ش�����̬��liblogpipe_api.so����������á�

Ȼ��װ����Ŀ�꣬Ĭ��`logpipe`��װ��`$HOME/bin`��`liblogpipe_api.so`��װ��`$HOME/lib`��`logpipe_api.h`��һЩͷ�ļ���װ��`$HOME/include/logpipe`�������Ҫ�ı䰲װĿ¼���޸�`makeinstall`���`_HDERINST`��`_LIBINST`��`_BININST`��

```
$ make -f makefile.Linux install
rm -f /home/calvin/bin/logpipe
cp -rf logpipe /home/calvin/bin/
rm -f /home/calvin/lib/liblogpipe_api.so
cp -rf liblogpipe_api.so /home/calvin/lib/
rm -f /home/calvin/include/logpipe/rbtree.h
cp -rf rbtree.h /home/calvin/include/logpipe/
rm -f /home/calvin/include/logpipe/LOGC.h
cp -rf LOGC.h /home/calvin/include/logpipe/
rm -f /home/calvin/include/logpipe/fasterjson.h
cp -rf fasterjson.h /home/calvin/include/logpipe/
rm -f /home/calvin/include/logpipe/rbtree_tpl.h
cp -rf rbtree_tpl.h /home/calvin/include/logpipe/
rm -f /home/calvin/include/logpipe/logpipe_api.h
cp -rf logpipe_api.h /home/calvin/include/logpipe/
```

### 2.1.2. ���밲װ�Դ�logpipe���

����`src`ͬ����`src-plugins`��������

```
$ cd ../src-plugins
$ make -f makefile.Linux 
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99 -I/home/calvin/include/logpipe  -c logpipe-input-file.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o logpipe-input-file.so logpipe-input-file.o -shared -L. -L/home/calvin/so -L/home/calvin/lib -llogpipe_api -rdynamic 
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99 -I/home/calvin/include/logpipe  -c logpipe-output-file.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o logpipe-output-file.so logpipe-output-file.o -shared -L. -L/home/calvin/so -L/home/calvin/lib -llogpipe_api -rdynamic 
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99 -I/home/calvin/include/logpipe  -c logpipe-input-tcp.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o logpipe-input-tcp.so logpipe-input-tcp.o -shared -L. -L/home/calvin/so -L/home/calvin/lib -llogpipe_api -rdynamic 
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -I. -I/home/calvin/include -std=gnu99 -I/home/calvin/include/logpipe  -c logpipe-output-tcp.c
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o logpipe-output-tcp.so logpipe-output-tcp.o -shared -L. -L/home/calvin/so -L/home/calvin/lib -llogpipe_api -rdynamic 
```

Ȼ��װ����Ŀ�꣬Ĭ���Դ������װ��`$HOME/so`�������Ҫ�ı䰲װĿ¼���޸�`makeinstall`���`_LIBINST`��

```
$ make -f makefile.Linux install
rm -f /home/calvin/so/logpipe-input-file.so
cp -rf logpipe-input-file.so /home/calvin/so/
rm -f /home/calvin/so/logpipe-output-file.so
cp -rf logpipe-output-file.so /home/calvin/so/
rm -f /home/calvin/so/logpipe-input-tcp.so
cp -rf logpipe-input-tcp.so /home/calvin/so/
rm -f /home/calvin/so/logpipe-output-tcp.so
cp -rf logpipe-output-tcp.so /home/calvin/so/
```

### 2.1.3. ȷ�ϰ�װ

ȷ��`$HOME/bin`�Ѿ����뵽`$PATH`�У���������ִ��`logpipe`�����������Ϣ��ʾԴ����밲װ�ɹ�

```
$ logpipe
USAGE : logpipe -v
        logpipe -f (config_file) [ --no-daemon ] [ --start-once-for-env "(key) (value)" ]
```

�ò���`-v`���Բ鿴��ǰ�汾��

```
$ logpipe -v
logpipe v0.7.0 build Dec 17 2017 20:02:46
```

# 3. ʹ��



# 4. �������



# 5. ����ѹ��



# 6. ���
