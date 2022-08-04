#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <sqlite3.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include "head.h"

#define PASSWORD "123456"                         //设置管理员密码
#define BUF_ZEROS(zer, max) memset(zer, 0, max)   //二维数组初始化宏
#define BUF_ZERO(zer) memset(zer, 0, sizeof(zer)) //一维数组初始化宏
struct ad_data all;
//指令码定义
char *menu[] = {"姓名", "年龄", "住址", "电话", "职位", "工资", "入职时间", "评级", "密码"};
char *menu_two[] = {"住址", "电话", "密码"};
int menu_int[9] = {1, 4, 6, 5, 7, 10, 8, 9, 2};
int menu_two_int[3] = {6, 5, 2};

//目录循环条件初始化
#define UP_ONE (buf[0]=buf[1]=buf[2]=buf[3]=buf[4]=buf[5]=1)
/*****************************************提示消息**************************************************/
/****
    tips[0]="请输入工号:"                                       tips[1]="请输入用户名>>>>"
    tips[2]="请输入用户密码(2-6位)>>>"                           tips[3]="请输入该员工性别:"
    tips[4]="请输入年龄："                                      tips[5]="请输入电话：",
    tips[6]="请输入家庭住址："                                   tips[7]="请输入职位："
    tips[8]="请输入入职日期（格式:xxxx:xx:xx):"                  tips[9]="请输入评级(1~5,5为最高,新员工为1):"
    tips[10]="请输入工资："                                      tips[11]="你输入的工号是："
    tips[12]="工号信息只要录入将无法修改,请确认是否正确!(Y,N):"     tips[13]="是否设置为管理员:(Y/N):"
    tips[14]="数据添加成功，是否继续添加员工：(Y/N):"              tips[15]="请输入要删除的用户工号:"
    tips[16]="亲爱的管理员，欢迎登录员工管理系统！"                tips[17]="亲爱的员工，欢迎登录员工管理系统！"
    tips[18]="请输入你的选择>>>>>"                               tips[19]="登陆成功"
    tips[20]="请输入你要查询的用户名>>>"                          tips[21]="请输入你要修改的员工号>>>"
    tips[22]="数据修改成功，修改结束！"                           tips[23]="请输入要删除的用户名："
    tips[24]="输入有误，请确认该员工是否存在！"                    tips[25]="数据修改成功！删除工号为:"
    tips[26]="历史记录查询结束！"                                 tips[27]="暂无该员工信息，请联系管理员添加"
    tips[28]="请输入添加管理员权限密码："                          tips[29]="是否有管理员账号："
    tips[30]="添加数据失败"                                       tips[31]="管理员数据添加成功，是否登录(Y/N):"
    tips[32]="账户或密码错误"                                     tips[33]="密码错误，无权限添加"
    tips[34]="该工号已存在，是否重新输入(Y/N)"                      tips[35]="输入有误，请重新输入："
    tips[36]="查询结果失败"                                        tips[37]="员工信息查询完成"
    tips[38]="操作失败"                                            tips[39]="该用户已登录,请勿重复登录"
******/
unsigned char *tips[] = {
    "请输入工号:", "请输入用户名>>>>", "请输入用户密码(2-6位)>>>", "请输入该员工性别:",
    "请输入年龄：", "请输入电话：", "请输入家庭住址：", "请输入职位：",
    "请输入入职日期（格式:xxxx:xx:xx):", "请输入评级(1~5,5为最高,新员工为1):", "请输入工资：",
    "你输入的工号是：", "工号信息只要录入将无法修改,请确认是否正确!(Y,N):",
    "是否设置为管理员:(Y/N):", "数据添加成功，是否继续添加员工：(Y/N):", "请输入要删除的用户工号:",
    "亲爱的管理员，欢迎登录员工管理系统！", "亲爱的员工，欢迎登录员工管理系统！", "请输入你的选择>>>>>",
    "登陆成功", "请输入你要查询的用户名>>>", "请输入你要修改的员工号>>>", "数据修改成功，修改结束！",
    "请输入要删除的用户名：", "输入有误，请确认该员工是否存在！", "数据修改成功！删除工号为:",
    "历史记录查询结束！", "暂无该员工信息，请联系管理员添加", "请输入添加管理员权限密码：",
    "是否有管理员账号：", "添加数据失败", "管理员数据添加成功，是否登录(Y/N):", "账户或密码错误",
    "密码错误，无权限添加", "用户名登录/工号登录(0/1):", "该工号已存在，是否重新输入(Y/N)",
    "输入有误，请重新输入：","查询结果失败","员工信息查询完成","操作失败","该用户已登录,请勿重复登录"};
/************************************************************************************************************/

/**********************客户端交互数据处理函数*************************/
//分线程处理函数(数据交互)
void *data_interaction(void *arg)
{
    int buf[6] = {1, 1, 1, 1, 1, 1};     //结束处理函数的条件
    sqlite3 *db = NULL;                  //数据哭的句柄
    struct msg prd = *(struct msg *)arg; //将线程接收到的形参转换为需要的参数
    int sfd = prd.newfd;
    char nbuf[128];
    int falg = 1;
    struct ad_data name;  //保存当前线程下用户的工号、姓名
    int usertypoe;                  //用于标志当前进程下用户的类型1、管理员/2、普通用户
    pthread_detach(pthread_self()); //分离线程
    BUF_ZEROS(&name,256);
    UP_ONE;     //初始化循环条件
    //数据库初始化
    db = sqlite_db();
    if (db == NULL)
    {
        PERROR_ERR("open_db");
    }
    //总循环处理流程
    while (buf[0])
    {
        //一级目录处理流程
        while (buf[1])
        {
            send(sfd, "01", 2, 0);      //阻塞方式发送一级目录
            BUF_ZERO(nbuf);
            if (recv(sfd, nbuf, sizeof(nbuf), 0)== 0){
                buf[0]=buf[1] = 0;break;}
            if (strlen(nbuf) == 1)
            {
                switch (nbuf[0])
                {
                case '1': //管理员登录;
                    usertypoe = 1;
                    if(sqlite3_long(db, sfd, usertypoe)==0){    //调用登录函数
                    sprintf(&name.admin_all[0][0], "%s",&all.admin_all[0][0]); //保存工号
                    sprintf(&name.admin_all[1][0], "%s",&all.admin_all[1][0]); //保存用户名
                    buf[1]=0;}
                    break;
                case '2': //员工登录
                    usertypoe = 0;
                    if(sqlite3_long(db, sfd, usertypoe)==0){
                    sprintf(&name.admin_all[0][0], "%s",&all.admin_all[0][0]); //保存工号
                    sprintf(&name.admin_all[1][0], "%s",&all.admin_all[1][0]); //保存用户名
                    buf[1]=0;}
                    break;
                case '3':
                    buf[0] = buf[1] = 0;
                    break;
                default:
                    send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                }
            }
            else
            {
                send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
            }
        }
        //二级目录处理流程
        while (buf[2] == 0)
        {
            //管理员处理流程
            if (usertypoe == 1)
            {
                send(sfd, "02", 2, 0); //管理员一级目录指令码
                while (buf[3])
                {
                    BUF_ZERO(nbuf);
                    if (recv(sfd, nbuf, sizeof(nbuf), 0) <= 0){
                        buf[0] =buf[1]=buf[2]= 0;break;}
                    if (strlen(nbuf) == 1)
                    {
                        switch (nbuf[0])
                        {
                        case '1': //查询函数;
                            //管理员二级目录查找
                            while (buf[4])
                            {
                                BUF_ZERO(nbuf);
                                send(sfd,"03",2,0);     
                                if(recv(sfd,nbuf,sizeof(nbuf),0)==0){
                                    buf[3]=buf[2]=buf[1]=buf[0]=0;break;}
                                
                                sqlite3_lookup(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                            }
                            break;
                        case '2': //修改
                            //管理员二级目录修改
                            while (buf[5])
                            {
                                BUF_ZERO(nbuf);
                                send(sfd,"04",2,0); 
                                if(recv(sfd,nbuf,sizeof(nbuf),0)==0){
                                    buf[3]=buf[2]=buf[1]=buf[0]=0;break;}
                                sqlite3_revise(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                            }
                            break;
                        case '3': //添加用户信息
                            sqlite3_add(db,sfd,name);
                            break;
                        case '4': //删除用户
                            sqlite3_del(db,sfd,name);
                            break;
                        case '5': //查找历史记录
                            sqlite3_lookup(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                            break;
                        case '6': //退出
                            buf[2]=buf[3]= 0;
                            break;
                        default:
                            send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                        }
                    }
                    else
                    {
                        send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                    }
                }
            }
            else if (usertypoe == 0)
            {
                //员工一级目录
                while (buf[3])
                {
                    send(sfd, "05", 2, 0); //员工一级目录
                    BUF_ZERO(nbuf);
                    if (recv(sfd, nbuf, sizeof(nbuf), 0) <= 0){
                        buf[0]=buf[3] = 0;break;}
                    if (strlen(nbuf) == 1)
                    {
                        switch (nbuf[0])
                        {
                        case '1': //查找;
                            sqlite3_lookup(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                            break;
                        case '2': //修改
                            while (buf[4])
                            {
                                //员工二级目录修改
                                send(sfd, "06", 2, 0);
                                BUF_ZERO(nbuf);
                                if (recv(sfd, nbuf, sizeof(nbuf), 0) == 0){
                                    buf[4]=buf[3]=buf[0]=0;break;}
                                if (strlen(nbuf) == 1)
                                {
                                    if (nbuf[0] == '3')
                                    {
                                        buf[4] = 0;
                                    }
                                    else if((nbuf[0]=='1')||(nbuf[0]=='2'))
                                    { 
                                        sqlite3_revise(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                                    } 
                                    else{
                                    send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                                    }  
                                }
                                else
                                {
                                    send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                                }
                            }
                            break;
                        case '3': //退出
                            buf[2] =buf[3]= 0;
                            break;
                        default:
                            send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                        }
                    }
                    else
                    {
                        send(sfd, tips[35], strlen(tips[35]), 0); //选择错误提示
                    }
                }
            }
        }
        UP_ONE;     //初始化循环条件
    }
    printf("客户端用户%s>>>%s:%d\n",&name.admin_all[1][0], inet_ntoa(prd.cin.sin_addr), ntohs(prd.newfd));
    printf("已断开连接\n");
    //删除函数
    dele_ad(db,name);
    close(sfd);
    sqlite3_close(db);
    pthread_exit(NULL);
}
/*********************************************************/
//删除
void dele_ad(sqlite3* db,struct ad_data id)
{
    char buf[128];
    time_t t=0;
    struct tm *time_cat=NULL;
    char *errmsg = NULL;
    BUF_ZERO(buf);
    sprintf(buf,"delete from stu3 where \"工号\"=\"%s\"",&id.admin_all[0][0]);
    if(sqlite3_exec(db,buf,NULL,NULL,&errmsg)!=SQLITE_OK){
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return;
    }
     //保存操作记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(buf);
    sprintf(buf, "insert into stu2 values(\"%s\",\"%s\",\"退出登录\",\"%d月  %d日  %02d:%02d\")",&id.admin_all[0][0],\
            &id.admin_all[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return;
    }
}
/*********************TCP服务器配置****************/
//服务器初始化
int tcp_init(void)
{
    char buf[20] = {"192.168.250.100"}; //保存ip地址
    int port = 8888;                    //保存端口信息
#if 0                                   //调试的时候禁用
    //提示当前服务器ip信息
    system("ifconfig");
    printf("请输入本机IP地址>>>>>>>>");
    fgets(buf, sizeof(buf), stdin);
    printf("请输入端口号port>>>>>>>");
    scanf("%d", &port);
    while (getchar() != 10);
#endif
    //创建套接字
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        PERROR_ERR("socket error");
        return -1;
    }
    //设置允许端口快速重用
    int resin = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &resin, sizeof(resin)) < 0)
    {
        PERROR_ERR("setsockopt error");
        return -1;
    }
    //设置结构体信息
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);           //设置端口信息
    sin.sin_addr.s_addr = inet_addr(buf); //设置ip信息
    //绑定信息
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        PERROR_ERR("bind errpr");
        return -1;
    }
    //设为被动监听状态
    if (listen(sd, 20) < 0)
    {
        PERROR_ERR("listen error");
        return -1;
    }
    return sd;
}
/*********************************************************/

/**************数据库的配置函数***************************/
//数据库初始化创建并打开
sqlite3 *sqlite_db(void)
{
    sqlite3 *db;
    unsigned char buf[128]; //指令信息
    char *errmsg = NULL;
    //打开数据库
    if (sqlite3_open("./new_data.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "__%d__,sqlite3_open:%s\n", __LINE__, sqlite3_errmsg(db));
        return NULL;
    }
    BUF_ZERO(buf);
    //创建基础信息表(存在不执行该语句不存在执行该语句)
    /****************************************表头******************************************/
    /*****工号、姓名、密码、性别、年龄、联系方式、家庭住址、公司职位、入职时间、评级、工资********/
    /*sprintf(buf, "create table if not exists stu1(ip char primary key,name char,\
            password char,sex char,age char,contact char,address char,\
            position char,entry_time char,grade char,wages char)");
    sprintf(buf, "create table if not exists stu1(工号 char primary key,姓名 char,\
            密码 char,性别 char,年龄 char,电话 char,住址 char,\
            职位 char,入职时间 char,评级 char,工资 char)");*/
    if (sqlite3_exec(db, "create table if not exists stu1(\"用户类型\" int,\"工号\" char primary key,\"姓名\" char,\
            \"密码\" char,\"性别\" char,\"年龄\" char,\"电话\" char,\"住址\" char,\
            \"职位\" char,\"入职时间\" char,\"评级\" char,\"工资\" char)",
                     NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "%s:__%d__,sqlite3_exec:%s\n", __FILE__, __LINE__, errmsg);
        return NULL;
    }

    //创建修改历史记录表
    BUF_ZERO(buf);
    sprintf(buf, "create table if not exists stu2(\"工号\" char,\"姓名\" char,\"修改记录\" char,\"修改时间\" char)");
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return NULL;
    }

     //创建登录用户表格
    BUF_ZERO(buf);
    sprintf(buf, "create table if not exists stu3(\"工号\" char primary key,\"姓名\" char)");
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return NULL;
    }
    return db;
}
/*********************************************************/

/**************数据库操作函数***************************/
//登录(管理员登录/普通登录)
int sqlite3_long(sqlite3 *db, int fsd, int usertypoe)
{
    char **result = NULL;
    int row,column;
    char *errmsg = NULL;
    char buf[128];
    struct inifo st;
    time_t t = 0;
    struct tm *time_cat = NULL;
    BUF_ZEROS(&st,1536);
    //管理员判断
    if (usertypoe == 1)
    {
        //是否添加管理员
        send(fsd, tips[29], strlen(tips[29]), 0); //提示是否有管理员账号
        if (recv(fsd, buf, sizeof(buf), 0) <= 0)
            return -1;
        if (strncasecmp(buf, "n", 1) == 0)
        {
            //调用管理员添加函数     
            if (admin_add(db, fsd))
            {
                send(fsd, tips[30], strlen(tips[30]), 0); //添加失败
            }
            BUF_ZERO(buf);
            send(fsd, tips[31], strlen(tips[31]), 0); //添加成功，提示是否立即登录
            if (recv(fsd, buf, sizeof(buf), 0) < 0)
                return -1;
        }
    }
    if((usertypoe==0)||(strncasecmp(buf, "N", 1) == 0)){
    //登录函数
    BUF_ZERO(buf);
    send(fsd, tips[0], strlen(tips[0]), 0);         //工号
    if (recv(fsd, &st.new_buf[0][0],128, 0) == 0)   
        return -1;
    send(fsd,tips[2],strlen(tips[2]),0);
    if(recv(fsd,&st.new_buf[1][0],128,0)==0)       //密码
        return -1;
    //密码工号匹配
    sprintf(buf,"select * from stu1 where 工号=\"%s\"",&st.new_buf[0][0]);
   if (sqlite3_get_table(db,buf, &result, &row, &column, &errmsg) != SQLITE_OK)     //获取密码工号
    {
        send(fsd,tips[38],strlen(tips[38]),0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    if((row&&column)==0){
        send(fsd,tips[27],strlen(tips[27]),0);     //暂无用户信息
        return -1;
    }else{
        //密码匹配
        if(strcasecmp(result[15],&st.new_buf[1][0])!=0){
            send(fsd,tips[32],strlen(tips[32]),0);      //密码错误
            return -1;
        }
    }
    sprintf(&all.admin_all[0][0], "%s", &st.new_buf[0][0]); //保存登录用户工号信息
    sprintf(&all.admin_all[1][0], "%s", &st.new_buf[1][0]); //保存登录用户姓名信息
    }else{
    sprintf(&st.new_buf[0][0], "%s", &all.admin_all[0][0]); //保存登录用户工号信息
    sprintf(&st.new_buf[1][0], "%s", &all.admin_all[1][0]); //保存登录用户姓名信息
    }
    //防止重复登录
    BUF_ZERO(buf);
    sprintf(buf,"insert into stu3 values(\"%s\",\"%s\")",&st.new_buf[0][0],&st.new_buf[1][0]);
    if(sqlite3_exec(db,buf,NULL,NULL,&errmsg)!=SQLITE_OK){
        send(fsd,tips[39],strlen(tips[39]),0);              //重复登录提示
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    //保存操作记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(buf);
    sprintf(buf, "insert into stu2 values(\"%s\",\"%s\",\"登陆成功\",\"%d月  %d日  %02d:%02d\")",&st.new_buf[0][0],\
            &st.new_buf[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],strlen(tips[38]),0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    if(usertypoe==1){
    send(fsd,tips[16],strlen(tips[16]),0);}
    else{
    send(fsd,tips[17],strlen(tips[17]),0);}
    return 0;
}

//管理员添加函数
int admin_add(sqlite3 *db, int fsd)
{
    char buf[128];
    char *errmsg = NULL;
    int usertypoe = 1;
    struct inifo st;
    time_t t = 0;
    struct tm *time_cat = NULL;
    BUF_ZERO(buf);
    BUF_ZEROS(&st,1536);
    send(fsd, tips[28], strlen(tips[28]), 0); //验证添加管理员权限密码
    if (recv(fsd, buf, sizeof(buf), 0) <= 0)
        return -1;
    if (strncasecmp(buf, PASSWORD, 6) == 0)
    {
        for (int i = 0; i < 11; i++)
        {
            //添加管理员信息
            send(fsd, tips[i], strlen(tips[i]), 0);
            printf("%s", tips[i]);
            fflush(stdout);
            if (recv(fsd, &st.new_buf[i][0],128, 0) == 0)
                    return -1;
                printf("%s\n",&st.new_buf[i][0]);
        }
        //保存到数据库
        BUF_ZERO(buf);
        sprintf(buf, "insert into stu1 values(%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\")",\
                usertypoe, &st.new_buf[0][0], &st.new_buf[1][0], &st.new_buf[2][0],&st.new_buf[3][0], &st.new_buf[4][0],\
                &st.new_buf[5][0], &st.new_buf[6][0], &st.new_buf[7][0],&st.new_buf[8][0], &st.new_buf[9][0], &st.new_buf[10][0]);
        if (sqlite3_exec(db,buf,NULL,NULL,NULL) != SQLITE_OK)
        {
            send(fsd,tips[38],strlen(tips[38]),0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
    }
    else
    {
        send(fsd, tips[33], strlen(tips[33]), 0);
        return -1;
    }
    //保存操作记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(buf);
    sprintf(buf, "insert into stu2 values(\"%s\",\"超级BOOS\",\"BOOS添加了%s为管理员\",\"%d月  %d日  %02d:%02d\")","000000",\
            &st.new_buf[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,\
            time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],strlen(tips[38]),0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    printf("%ld\t%ld\n",strlen(&st.new_buf[0][0]),strlen(st.new_buf[1]));
    sprintf(&all.admin_all[0][0], "%s", &st.new_buf[0][0]); //保存登录用户工号信息
    sprintf(&all.admin_all[1][0], "%s", &st.new_buf[1][0]); //保存登录用户姓名信息
    printf("管理员信息添加成功\n");
    return 0;
}

//添加信息
int sqlite3_add(sqlite3 *db, int fsd, struct ad_data id)
{
    char buf[128] = {"\0"};
    char out;
    int falg = 1;
    int usertypoe = 0;
    char *errmsg = NULL;
    struct inifo st;
    time_t t = 0;
    struct tm *time_cat = NULL;
    BUF_ZEROS(&st,1536);
    BUF_ZEROS(&id,256);
    do
    {
        for (int i = 0; i < 11; i++)
        {
            //添加基本员工员信息
            send(fsd, tips[i], strlen(tips[i]), 0);
            recv(fsd, &st.new_buf[i][0], 128, 0);
            printf("%s%s\n", tips[i], &st.new_buf[i][0]);
        }
        //判断是否需要修改工号
        BUF_ZERO(buf);
        sprintf(buf, "%s<%s>,%s", tips[11], &st.new_buf[0][0], tips[12]);
        send(fsd, buf, sizeof(buf), 0);
        if (recv(fsd, &out, 1, 0) < 0)
            return -1;
        if (strcasecmp(&out, "Y") != 0)
        {
            do
            {
                //判断工号是否重复
                BUF_ZERO(buf);
                sprintf(buf, "insert into stu1 (\"工号\") values(\"%s\")", &st.new_buf[0][0]);
                if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
                {
                    send(fsd, tips[34], strlen(tips[34]), 0);
                    if (recv(fsd, &out, 1, 0) < 0)
                        return -1;
                    if ((out == 'Y') || (out == 'y'))
                    {
                        //清除工号信息
                        BUF_ZEROS(&st.new_buf[0][0], 128);
                        //重新获取工号
                        send(fsd, tips[0], strlen(tips[0]), 0);
                        if (recv(fsd, &st.new_buf[0][0], sizeof(&st.new_buf[0][0]), 0) < 0)
                            return -1;
                    }
                    else
                    {
                        out = 0; //结束循环的条件
                    }
                }
                out = 0; //结束循环条件
            } while (out);
        }
        //判断是否设置为管理员
        BUF_ZERO(buf);
        send(fsd, tips[13], strlen(tips[13]), 0);
        if (recv(fsd, &out, 1, 0) < 0)
            return -1;
        if (strcasecmp(&out, "Y") == 0)
            usertypoe = 1;
        //保存到数据库
        BUF_ZERO(buf);
        sprintf(buf, "insert into stu1 values(%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\
        \"%s\",\"%s\",\"%s\",\"%s\",\"%s\")",\
                usertypoe, &st.new_buf[0][0], &st.new_buf[1][0], &st.new_buf[2][0],\
                &st.new_buf[3][0], &st.new_buf[4][0], &st.new_buf[5][0], &st.new_buf[6][0], &st.new_buf[7][0],\
                &st.new_buf[8][0], &st.new_buf[9][0], &st.new_buf[10][0]);
        if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],strlen(tips[38]),0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        //保存操作记录
        time(&t);                 //获取时间
        time_cat = localtime(&t); //转换时间
        BUF_ZERO(buf);
        sprintf(buf, "insert into stu2 values(\"%s\",\"管理员%s\",\"添加了%s的员工\",\"%d月  %d日  %02d:%02d\")", &id.admin_all[0][0], &id.admin_all[1][0], \
                &st.new_buf[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
        if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],strlen(tips[38]),0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
    } while (falg);
    send(fsd,tips[14],strlen(tips[14]),0);
    if(recv(fsd,&out,1,0)==0)
        return -1;
    if(strcasecmp(&out,"y")==0)
        sqlite3_add(db,fsd,id);
    printf("管理员添加了员工信息成功\n");
    return 0;
}

//删除信息
int sqlite3_del(sqlite3 *db, int fsd, struct ad_data id)
{
    char *errmsg = NULL;
    char buf[10] = {"\0"};
    char ubuf[128] = {"\0"};
    time_t t = 0;
    struct tm *time_cat = NULL;
    //获取要删除的工号
    BUF_ZERO(buf);
    send(fsd, tips[15], strlen(tips[15]), 0);
    if (recv(fsd, buf, sizeof(buf), 0) < 0)
        return -1;
    //执行删除语句
    BUF_ZERO(ubuf);
    sprintf(ubuf, "delete from stu1 where \"工号\"=\"%s\"", buf);
    if (sqlite3_exec(db, ubuf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],strlen(tips[38]),0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    //删除成功
    send(fsd, tips[25], strlen(tips[25]), 0);
    //保存操作历史记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(ubuf);
    sprintf(ubuf, "insert into stu2 values(\"%s\",\"管理员\"%s\"\",\"%s%s的员工\",\"%d月  %d日  %02d:%02d\")",&id.admin_all[0][0],&id.admin_all[1][0], tips[25],\
            buf, time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],strlen(tips[38]),0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    printf("管理员%s删除员工信息成功\n",&id.admin_all[1][0]);
    return 0;
}

//修改信息
int sqlite3_revise(sqlite3 *db, int fsd, struct ad_data id, int usertypoe, char falg)
{
    /*指令码
    char *menu[] = {"姓名", "年龄", "住址", "电话", "职位", "工资", "入职时间", "评级", "密码"};
    char *menu_two[] = {"住址","电话","密码"};
    int menu_int[9] = {1,4,6,5,7,10,8,9,2};
    int menu_two_int[3] = {6,5,2};*/
    time_t t = 0;
    struct tm *time_cat = NULL;
    char nbuf[128];   //指令存储
    char buf[2][128]; //修改信息
    char *errmsg = NULL;
    char *p=NULL;
    //初始化数组
    BUF_ZERO(nbuf);
    BUF_ZEROS(buf, 256);
   
    //判断是管理员修改还是普通员工修改
    if (usertypoe == 1)
    {
        //管理员修改程序
         send(fsd, tips[21], strlen(tips[21]), 0); //获取工号
        if (recv(fsd, &buf[0][0], 128, 0) == 0)
        return -1;
        send(fsd, tips[(menu_int[falg])], strlen(tips[(menu_int[falg])]), 0);
        if (recv(fsd, &buf[1][0], 128, 0) == 0)
            return -1;
        sprintf(nbuf, "update stu1 set \"%s\"=\"%s\" where 工号=\"%s\"", menu[falg], &buf[1][0], &buf[0][0]);
        if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],strlen(tips[38]),0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        p=menu_two[falg];
        send(fsd, tips[22], strlen(tips[22]), 0);
    }
    else if (usertypoe == 0)
    {
        //员工修改程序
        send(fsd, tips[(menu_two_int[falg])], strlen(tips[(menu_two_int[falg])]), 0);
        if (recv(fsd, &buf[1][0], 128, 0) == 0)
            return -1;
        sprintf(nbuf, "update stu1 set \"%s\"=\"%s\" where 工号=\"%s\"", menu_two[falg], &buf[1][0], &id.admin_all[0][0]);
        if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],strlen(tips[38]),0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        p=menu[falg];
        send(fsd, tips[22], strlen(tips[22]), 0);
    }
    //保存记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(nbuf);
   
    sprintf(nbuf, "insert into stu2 values(\"%s\",\"%s\",\"修改%s为%s\",\"%d月  %d日  %02d:%02d\")", &id.admin_all[0][0], &id.admin_all[1][0],p,\
            &buf[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],strlen(tips[38]),0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    printf("用户%s修改了%s为%s\n",&id.admin_all[1][0],p,&buf[1][0]);
    return 0;
}

//查找用户信息
int sqlite3_lookup(sqlite3 *db, int fsd, struct ad_data id, int usertypoe, int falg)
{
    int index = 0;
    char sql[128] = {"\0"};
    char **result = NULL;
    int row, column;
    char *errmsg = NULL;
    char buf[128] ={"\0"};

    BUF_ZERO(sql);
    BUF_ZERO(buf);
    //判断是管理员还是普通员工
    if (usertypoe == 1)
    {
        if (falg == 1)
        {
            send(fsd, tips[1], strlen(tips[1]), 0);
            if(recv(fsd,buf,sizeof(buf),0)==0)
                return -1;
            sprintf(sql,"select * from stu1 where 姓名=\"%s\"",buf);
            //管理员查找程序
            if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
            {
                send(fsd,tips[36],strlen(tips[36]),0);
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return -1;
            }
           
        }else if(falg == 2){
            sprintf(sql,"select * from stu1");
            //管理员查找程序
            if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
            {
                send(fsd,tips[36],strlen(tips[36]),0);
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return -1;
            }
        }else if(falg == 5){
            sprintf(sql,"select * from stu2");
            //管理员查找程序
            if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
            {
                send(fsd,tips[36],strlen(tips[36]),0);
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return -1;
            }
        }
    }
    else if (usertypoe == 0)
    {
        //员工查找程序
        sprintf(sql,"select * from stu1 where 工号=\"%s\"",&id.admin_all[0][0]);
        //管理员查找程序
        if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[36],strlen(tips[36]),0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
    }
     BUF_ZERO(buf);
    //发送数据
    if(row&&column)
    {
        if(falg!=5){
        //用户信息处理
        for(int i=0;i<=row;i++){
            sprintf(buf,"%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s",result[(index)],9,result[(index+1)],\
            9,result[(index+2)],9,result[(index+3)],9,result[(index+4)],9,result[(index+5)],9,result[(index+6)],9,result[(index+7)],\
            9,result[(index+8)],9,result[(index+9)],9,result[(index+10)],9,result[(index+11)]);
            send(fsd,buf,sizeof(buf),0);
            BUF_ZERO(buf);
            index = index+12;
        }
        }else{
            //历史记录处理
            for(int i=0;i<=row;i++){
            sprintf(buf,"%s%c,%s%c,%s%c,%s",result[(index)],9,result[(index+1)],9,result[(index+2)],9,result[(index+3)]);
            send(fsd,buf,sizeof(buf),0);
            BUF_ZERO(buf);
            index = index+4;
            }
        }
    }else{send(fsd,tips[36],strlen(tips[36]),0);
            printf("信息查询失败\n");
            return -1;
        }
    printf("信息查询成功\n");
    send(fsd,tips[37],strlen(tips[37]),0);
    return 0;
}
/*********************************************************/