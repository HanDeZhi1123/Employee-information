#ifndef __HEAD_H
#define __HEAD_H

//错误信息打印宏
#define PERROR_ERR(msg)  do{                                       \
        printf("<%s>---<%s:%d>\n", __FILE__, __func__, __LINE__); \
        perror(msg);                                              \
    } while (0)


// tcp连接后的信息
struct msg
{
    int newfd;              //新的文件描述符
    struct sockaddr_in cin; //保存连接的客户端信息
};


//员工信息结构体
struct inifo
{
    /***工号new_buf[0][0]********
        姓名new_buf[1][0]*********
        密码new_buf[2][0]*********
        性别new_buf[3][0]*********
        年龄new_buf[4][0]*********
        联系方式new_buf[5][0]*****
        家庭住址new_buf[6][0]*****
        公司职位new_buf[7][0]*****
        入职时间new_buf[8][0]*******
        评级new_buf[9][0]**********
        工资new_buf[10][0]********
        员工类型new_buf[11][0]****/
    unsigned char new_buf[12][128];
};
struct ad_data{
   unsigned char admin_all[2][128];     //保存登录用户工号、姓名
};


// tcp初始化函数声明
int tcp_init(void);

//数据库初始化函数声明
sqlite3 *sqlite_db(void);
//登录函数
int sqlite3_long(sqlite3 *db, int fsd,int usertypoe);
//管理员添加函数
int admin_add(sqlite3* db,int fsd);
//添加信息
int sqlite3_add(sqlite3 *db,int fsd,struct ad_data id);
//删除函数
int sqlite3_del(sqlite3 *db,int fsd,struct ad_data id);
//修改信息
int sqlite3_revise(sqlite3 *db,int fsd,struct ad_data id,int usertypoe,char falg);
//查找信息
int sqlite3_lookup(sqlite3 *db,int fsd,struct ad_data id,int usertypoe,int falg);

//分线程处理函数（数据交互）
void *data_interaction(void *arg);
//删除
void dele_ad(sqlite3* db,struct ad_data id,int falg);

#endif