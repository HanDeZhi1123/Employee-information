#include<stdio.h>
#include<strings.h>
#include<unistd.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<sys/select.h>
#include "head.h"

int main()
{
	int i,select_res;
	int res = 0;
	//准备函数
	int sfd = init();
	//创建集合
	fd_set IO_input,IO_input_all;

	//初始化机集合
	FD_ZERO(&IO_input_all);
	FD_ZERO(&IO_input);

	//添加集合
	FD_SET(0,&IO_input_all);
	FD_SET(sfd,&IO_input_all);
	
	//将最大的文件描述符保存
	int max = sfd;

	while(1)
	{
		IO_input = IO_input_all;
		//监听集合
		select_res = select(max+1,&IO_input,NULL,NULL,NULL);
		if(select_res <0)
		{
			PERROR_ERR("select");
			return 0;
		}
		
		//判断是否发生相应的事件
	for(i=0;i<max+1;i++)
		{
			if(!FD_ISSET(i,&IO_input))
			{
				continue;
			}
			if(0 == i)
			{
				//发送数据
				send_out(sfd);
				i = max+1;
			}
			else if(sfd == i)
			{
				//接收数据
				res = recv_input(sfd);
				if(res < 0)
				{
					goto END;	
				}
				i = max+1;
			}
		}
	}
END:
	printf("已断开连接\n");
	close(sfd);
    return 0;
}