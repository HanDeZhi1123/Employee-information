#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <unistd.h>
#include "head.h"

int main(int argc,const char* argv[])
{
    //创建结构体的信息
    int fsd;  
    struct msg prd;
    pthread_t tid;
    int len = sizeof(prd.cin);
    //tcp信息的初始化
    fsd = tcp_init();
    if(fsd<0){
        printf("初始化失败\n");
        return -1;
    }
    //通讯
    while(1){
        //等待客户端连接
        prd.newfd = accept(fsd,(struct sockaddr *)&prd.cin,&len);
        if(prd.newfd<0){
            PERROR_ERR("accept");
            return -1;
        }
        //创建线程实现并发服务器
        pthread_create(&tid,NULL,data_interaction,&prd);
        
    }
    close(fsd);
    return 0;
}