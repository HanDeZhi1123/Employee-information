#ifndef __HEAD_H__
#define __HEAD_H__

//错误信息打印宏
#define PERROR_ERR(msg)  do{                                       \
        printf("<%s>---<%s:%d>\n", __FILE__, __func__, __LINE__); \
        perror(msg);                                              \
    } while (0)

//客户端连接初始化
int init(void);
//菜单
void menu(void);
//管理员一级目录
void menu_admin(void);
//管理员二级目录查找
void menu_admin_lookup(void);
//管理员二级目录修改
void menu_admin_revise(void);
//普通员工一级目录
void menu_on(void);
//普通员工二级目录
void menu_on_revise(void);
//发送数据
void send_out(int sfd);
//接收数据
int recv_input(int sfd);

#endif