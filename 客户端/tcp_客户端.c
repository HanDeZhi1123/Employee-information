#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "head.h"

//客户端连接初始化
int init(void)
{
    char buf[20] = {"192.168.250.100"}; //保存ip地址
    int port = 8888;                    //保存端口信息
#if 0                                   //调试的时候禁用
    memset(buf,0,sizeof(buf));
    //提示当前服务器ip信息
    printf("请输入服务器的IP地址>>>>>>>>");
    fgets(buf, sizeof(buf), stdin);
    printf("请输入端口号port>>>>>>>");
    scanf("%d", &port);
    while (getchar() != 10);
#endif
    //创建套接字
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        printf("创建失败\n");
        return -1;
    }
    //填充结构体信息
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(buf);
    //允许端口快速重用
    int reuae = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuae, sizeof(reuae)) < 0)
    {
        perror("setsockopt");
        return 0;
    }

    //等待服务器打开
    do
    {
        if (connect(sfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            if (errno != 111)
            {
                perror("connect");
                return 0;
            }
            printf("等待服务器打开   \r");
            printf("等待服务器打开.\r");
            fflush(stdout);
            sleep(1);
            printf("等待服务器打开..\r");
            fflush(stdout);
        }
        else
        {
            errno = 0;
        }
        fprintf(stdout,"等待服务器打开...\r");
    } while ((errno == 111 && 3 == sfd));
    printf("已连接\n");
    return sfd;
}
//发送数据
void send_out(int sfd)
{
    char addr[128] = "\0";
    fgets(addr, sizeof(addr), stdin);
    addr[strlen(addr) - 1] = 0;
    send(sfd, addr, strlen(addr), 0);
    return;
}
//接收数据
int recv_input(int sfd)
{
    char buf[128] = "\0";
    ssize_t res = 0;
    memset(buf, 0, sizeof(buf));
    res = recv(sfd, buf, sizeof(buf), 0);
    if (res == 0)
    {
        return -1;
    }
    if (strlen(buf) <= 2)
    {
        switch (buf[0])
        {
        case '1':
            menu();
            break;
        case '2':
            menu_admin();
            break;
        case '3':
            menu_admin_lookup();
            break;
        case '4':
            menu_admin_revise();
            break;
        case '5':
            menu_on();
            break;
        case '6':
            menu_on_revise();
            break;
        }
    }
    else
    {
        //数据包直接在终端打印数据
        printf("%s", buf);
        fflush(stdout);
    }
    return 0;
}

//菜单
void menu(void)
{
    /************目录信息***************/
    /**********一级目录****************/
    printf("**********************************************************\n");
    printf("**************1、管理员   2、普通用户   3、退出***********\n");
    printf("**********************************************************\n");
    printf("请输入你的选择>>>>>>");
    fflush(stdout);
    return;
}
//管理员一级目录
void menu_admin(void)
{
    /*******管理员一级目录***************/
    printf("**********************************************************\n");
    printf("********1、查询       2、修改         3、添加用户*********\n");
    printf("********4、删除用户   5、查找历史记录  6、退出************\n");
    printf("**********************************************************\n");
    printf("请输入你的选择>>>>>>");
    fflush(stdout);
    return;
}
//管理员二级目录查找
void menu_admin_lookup(void)
{
    /*******管理员二级目录查找***************/
    printf("**********************************************************\n");
    printf("********1、按人名查找   2、查找所有     3、退出***********\n");
    printf("**********************************************************\n");
    printf("请输入你的选择>>>>>>");
    fflush(stdout);
    return;
}
//管理员二级目录修改
void menu_admin_revise(void)
{
    /*******管理员二级目录修改***************/
    printf("**********************************************************\n");
    printf("********1、姓名   2、年龄  3、家庭住址  4、电话***********\n");
    printf("********5、职位   6、工资  7、入职年月  8、评级***********\n");
    printf("********9、密码   10、退出                    ************\n");
    printf("**********************************************************\n");
    printf("请输入你的选择>>>>>>");
    fflush(stdout);
    return;
}
//普通员工一级目录
void menu_on(void)
{
    /*******普通员工一级目录***************/
    printf("**********************************************************\n");
    printf("**************1、查找   2、修改     3、退出***************\n");
    printf("**********************************************************\n");
    printf("请输入你的选择>>>>>>");
    fflush(stdout);
    return;
}
//普通员工二级目录
void menu_on_revise(void)
{
    /*******普通员工二级目录修改***************/
    printf("**********************************************************\n");
    printf("********1、家庭住址  2、联系电话  3、密码  4、退出********\n");
    printf("**********************************************************\n");
    printf("请输入你的选择>>>>>>");
    fflush(stdout);
    return;
}