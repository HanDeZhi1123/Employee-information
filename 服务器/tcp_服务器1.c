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
#define BUF_INIT(zer,al,max) memset(zer,al,max)   //初始化为你要的内容
struct ad_data all;
//指令码定义
char *menu[] = {"姓名", "年龄", "住址", "电话", "职位", "工资", "入职时间", "评级", "密码"};
char *menu_two[] = {"住址", "电话", "密码"};
int menu_int[9] = {1, 4, 6, 5, 7, 10, 8, 9, 2};
int menu_two_int[3] = {6, 5, 2};

//目录循环条件初始化
#define UP_ONE (buf[1]=buf[2]=buf[3]=buf[4]=buf[5]=1)
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
    tips[24]="删除失败，请确认该员工是否存在！"                    tips[25]="数据修改成功！删除工号为:"
    tips[26]="历史记录查询结束！"                                 tips[27]="暂无该员工信息，请联系管理员添加"
    tips[28]="请输入添加管理员权限密码："                          tips[29]="是否有管理员账号(Y/N)："
    tips[30]="添加数据失败"                                       tips[31]="管理员数据添加成功，是否登录(Y/N):"
    tips[32]="账户或密码错误"                                     tips[33]="密码错误，无权限添加"
    tips[34]="该工号已存在，是否重新输入(Y/N)"                      tips[35]="输入有误，请重新输入："
    tips[36]="查询结果失败"                                        tips[37]="员工信息查询完成"
    tips[38]="操作失败"                                            tips[39]="该用户已登录,请勿重复登录"
    tips[40]="添加失败，该工号已被占用\n"                           tips[41]="没有该用户信息，操作失败\n"
    tips[41]="该工号已存在,请重新输入\n"
******/
unsigned char *tips[] = {
    "请输入工号:", "请输入用户名>>>>", "请输入用户密码(2-6位)>>>", "请输入该员工性别:",
    "请输入年龄：", "请输入电话：", "请输入家庭住址：", "请输入职位：",
    "请输入入职日期（格式:xxxx:xx:xx):", "请输入评级(1~5,5为最高,新员工为1):", "请输入工资：",
    "你输入的工号是：", "工号信息只要录入将无法修改,请确认是否正确!(Y,N):",
    "是否设置为管理员:(Y/N):", "数据添加成功，是否继续添加员工：(Y/N):", "请输入要删除的用户工号:",
    "亲爱的管理员，欢迎登录员工管理系统！\n", "亲爱的员工，欢迎登录员工管理系统！\n", "请输入你的选择>>>>>",
    "登陆成功\n", "请输入你要查询的用户名>>>", "请输入你要修改的员工号>>>", "数据修改成功，修改结束！\n",
    "请输入要删除的用户名：", "删除失败，请确认该员工是否存在！\n", "数据修改成功！删除工号为:",
    "历史记录查询结束！\n", "暂无该员工信息，请联系管理员添加\n", "请输入添加管理员权限密码：",
    "是否有管理员账号(Y/N):", "添加数据失败\n", "管理员数据添加成功，是否登录(Y/N):", "账户或密码错误\n",
    "密码错误，无权限添加\n", "该工号已存在，是否重新输入(Y/N)","输入有误，请重新输入\n","查询结果失败\n",
    "员工信息查询完成\n","操作失败\n","该用户已登录,请勿重复登录\n","添加失败，该工号已被占用\n",
    "没有该用户信息，操作失败\n","该工号已存在,请重新输入\n"};
/**************************************************************************************************/

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
    int falg_out =1;
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
    //服务器打印提示信息
    printf("客户端用户%s:%d已连接服务器,正在尝试登陆\n",inet_ntoa(prd.cin.sin_addr), ntohs(prd.newfd));
    //总循环处理流程
    while (buf[0])
    {
        //一级目录处理流程
        while (buf[1])
        {
            UP_ONE;
            send(sfd, "1", 1, 0);      //阻塞方式发送一级目录
            BUF_ZERO(nbuf);
            if (recv(sfd, nbuf, sizeof(nbuf), 0)== 0){
                buf[0]=buf[1] = 0;break;}
            if (strlen(nbuf) == 1)
            {
                switch (nbuf[0])
                {
                case '1': //管理员登录
                    usertypoe = 1;
                    usertypoe=sqlite3_long(db, sfd, usertypoe);
                    if(usertypoe>=0){    //调用登录函数
                    sprintf(name.admin_all[0], "%s",all.admin_all[0]); //保存工号
                    sprintf(name.admin_all[1], "%s",all.admin_all[1]); //保存用户名
                    buf[1]=0;}
                    break;
                case '2': //员工登录
                    usertypoe = 0;
                    usertypoe = sqlite3_long(db, sfd, usertypoe);
                    if(usertypoe>=0){
                    sprintf(&name.admin_all[0][0], "%s",&all.admin_all[0][0]); //保存工号
                    sprintf(&name.admin_all[1][0], "%s",&all.admin_all[1][0]); //保存用户名
                    buf[1]=0;}
                    break;
                case '3':
                    buf[0] = buf[1] = buf[2]=0;
                    falg_out = 0;
                    break;
                default:
                    send(sfd, tips[35], 128, 0); //选择错误提示
                }
            }
            else
            {
                send(sfd, tips[35], 128, 0); //选择错误提示
            }
        }
        //二级目录处理流程
        while (buf[2])
        {
            //管理员处理流程
            if (usertypoe == 1)
            {
                
                while (buf[3])
                {
                    send(sfd, "2", 1, 0); //管理员一级目录指令码
                    BUF_ZERO(nbuf);
                    if (recv(sfd, nbuf, sizeof(nbuf), 0) == 0){
                        buf[0] =buf[1]=buf[2]=buf[3]= 0;break;}
                    if (strlen(nbuf) == 1)
                    {
                        switch (nbuf[0])
                        {
                        case '1': //查询函数;
                            //管理员二级目录查找
                            buf[4]=1;
                            while (buf[4])
                            {
                                BUF_ZERO(nbuf);
                                send(sfd,"3",1,0);    
                                if(recv(sfd,nbuf,sizeof(nbuf),0)==0){
                                    buf[3]=buf[2]=buf[1]=buf[0]=buf[4]=0;break;}
                                if(nbuf[0]!='3')
                                    sqlite3_lookup(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                                else
                                    buf[4]=0;
                            }
                            break;
                        case '2': //修改
                            //管理员二级目录修改
                            buf[5]=1;
                            while (buf[5])
                            {
                                BUF_ZERO(nbuf);
                                send(sfd,"4",1,0); 
                                if(recv(sfd,nbuf,sizeof(nbuf),0)==0){
                                    buf[3]=buf[2]=buf[1]=buf[0]=buf[5]=0;break;}
                                if((nbuf[0]=='1')&&(nbuf[1]=='0'))
                                    buf[5]=0; 
                                else if((nbuf[0]>'0')&&(nbuf[0]<'9'))
                                    sqlite3_revise(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                                else
                                    send(sfd,tips[35],128,0);
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
                            dele_ad(db,name,falg_out);
                            buf[2]=buf[3]= 0;
                            buf[1]=1;
                            break;
                        default:
                            send(sfd, tips[35], 128, 0); //选择错误提示
                        }
                    }
                    else
                    {
                        send(sfd, tips[35], 128, 0); //选择错误提示
                    }
                }
            }
            else if (usertypoe == 0)
            {
                //员工一级目录
                while (buf[3])
                {
                    send(sfd, "5", 1, 0); //员工一级目录
                    BUF_ZERO(nbuf);
                    if (recv(sfd, nbuf, sizeof(nbuf), 0) == 0){
                        buf[0]=buf[3]=buf[2] = 0;break;}
                    if (strlen(nbuf) == 1)
                    {
                        switch (nbuf[0])
                        {
                        case '1': //查找;
                            sqlite3_lookup(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                            break;
                        case '2': //修改
                            buf[4]=1;
                            while (buf[4])
                            {
                                //员工二级目录修改
                                send(sfd, "6", 1, 0);
                                BUF_ZERO(nbuf);
                                if (recv(sfd, nbuf, sizeof(nbuf), 0) == 0){
                                    buf[2]=buf[4]=buf[3]=buf[0]=0;break;}
                                if (strlen(nbuf) == 1)
                                {
                                    if (nbuf[0] == '4')
                                    {
                                        buf[4] = 0;
                                    }
                                    else if((nbuf[0]>='1')&&(nbuf[0]<='3'))
                                    { 
                                        sqlite3_revise(db,sfd,name,usertypoe,(int)(nbuf[0]-48));
                                    } 
                                    else{
                                    send(sfd, tips[35], 128, 0); //选择错误提示
                                    }  
                                }
                                else
                                {
                                    send(sfd, tips[35], 128, 0); //选择错误提示
                                }
                            }
                            break;
                        case '3': //退出
                            dele_ad(db,name,falg_out);
                            buf[2] =buf[3]= 0;
                            buf[1] = 1;
                            break;
                        default:
                            send(sfd, tips[35], 128, 0); //选择错误提示
                        }
                    }
                    else
                    {
                        send(sfd, tips[35], 128, 0); //选择错误提示
                    }
                }
            }
        }
    }
    printf("客户端用户%s>>>%s:%d\n",&name.admin_all[1][0], inet_ntoa(prd.cin.sin_addr), ntohs(prd.newfd));
    printf("已断开连接\n");
    //删除函数
    dele_ad(db,name,falg_out);
    close(sfd);
    sqlite3_close(db);
    pthread_exit(NULL);
}
/*********************************************************/
//删除
void dele_ad(sqlite3* db,struct ad_data id,int falg)
{
    char buf[128];
    time_t t=0;
    struct tm *time_cat=NULL;
    char *errmsg = NULL;
    if((strlen(id.admin_all[0])+strlen(id.admin_all[1]))>0){
    BUF_ZERO(buf);
    sprintf(buf,"delete from stu3 where \"工号\"=\"%s\"",&id.admin_all[0][0]);
    if(sqlite3_exec(db,buf,NULL,NULL,&errmsg)!=SQLITE_OK){
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return;
    }
    if(falg){
        //保存操作记录
        time(&t);                 //获取时间
        time_cat = localtime(&t); //转换时间
        BUF_ZERO(buf);
        sprintf(buf, "insert into stu2 values(\"%s\",\"%s\",\"退出登录\",\"%d月  %d日  %02d:%02d\")",&id.admin_all[0][0],\
            &id.admin_all[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
            if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK){
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return;
            }
        }
    }
}
/*********************TCP服务器配置****************/
//服务器初始化
int tcp_init(void)
{
    char buf[20] = {"192.168.250.100"}; //保存ip地址
    int port = 8888;                    //保存端口信息
#if 1                                   //调试的时候禁用
    memset(buf,0,sizeof(buf));
    //提示当前服务器ip信息
    system("ifconfig");
    printf("请输入本机IP地址>>>>>>>>");
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1]=0;
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
    int falg = 1;
    row=column=0;
    BUF_ZEROS(&st,1536);
    //send(fsd,tips[18],128[18]),0);
    //管理员判断
    if (usertypoe == 1)
    {
        do{
            BUF_ZERO(buf);
            //是否添加管理员
            send(fsd, tips[29], 128, 0); //提示是否有管理员账号
            if (recv(fsd, buf, sizeof(buf), 0) == 0)
                return -1;
            if(strlen(buf)==1)
                break;
            send(fsd,tips[35],128,0);
        }while(1);
        if (strncasecmp(buf, "n", 1) == 0)
        {
            //调用管理员添加函数   
            falg = admin_add(db, fsd);  
            if (falg==0){
            do{
                
                BUF_ZERO(buf);
                send(fsd, tips[31], 128, 0); //添加成功，提示是否立即登录
                if (recv(fsd, buf, sizeof(buf), 0) == 0)
                    return -1;
                if((strlen(buf)==1)){
                    if(strncasecmp(buf, "n", 1) == 0)
                    return -1;
                    if((strncasecmp(buf,"y",1)!=0))
                        send(fsd,tips[35],128,0);
                }else{send(fsd,tips[35],128,0);}
                falg = (strlen(buf)!=1)||(strncasecmp(buf,"y",1)!=0);
            }while(falg);
            }
            else 
                return -1;
        }else if(strncasecmp(buf, "y", 1) != 0){
            send(fsd,tips[35],128,0);
            return -1;
        }
    }
    if((usertypoe==0)||((strncasecmp(buf, "y", 1) == 0)&&(falg!=0))){
    //登录函数
    BUF_ZERO(buf);
    send(fsd, tips[0], 128, 0);       //工号
    if (recv(fsd, &st.new_buf[0][0],128, 0) == 0)   
        return -1;
    send(fsd,tips[2],128,0);
    if(recv(fsd,&st.new_buf[1][0],128,0)==0)       //密码
        return -1;
    //密码工号匹配
    sprintf(buf,"select * from stu1 where 工号=\"%s\"",&st.new_buf[0][0]);
   if (sqlite3_get_table(db,buf, &result, &row, &column, &errmsg) != SQLITE_OK)     //获取密码工号
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    if((row&&column)==0){
        printf("没有< %s >该用户信息\n",&st.new_buf[0][0]);
        send(fsd,tips[27],128,0);     //暂无用户信息
        return -1;
    }else{
        //密码匹配
        if(strcasecmp(result[15],&st.new_buf[1][0])!=0){
            send(fsd,tips[32],128,0);      //密码错误
            return -1;
        }
        printf("密码匹配成功\n");
    }
    sprintf(all.admin_all[0], "%s", st.new_buf[0]); //保存登录用户工号信息
    sprintf(all.admin_all[1], "%s", result[14]); //保存登录用户姓名信息
    }else{
        sprintf(st.new_buf[0], "%s", all.admin_all[0]); //保存登录用户工号信息
        sprintf(st.new_buf[1], "%s", all.admin_all[1]); //保存登录用户姓名信息
    }
    //防止重复登录
    BUF_ZERO(buf);
    sprintf(buf,"insert into stu3 values(\"%s\",\"%s\")",&st.new_buf[0][0],&st.new_buf[1][0]);
    if(sqlite3_exec(db,buf,NULL,NULL,&errmsg)!=SQLITE_OK){
        send(fsd,tips[39],128,0);              //重复登录提示
        return -1;
    }
    //保存操作记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(buf);
    sprintf(buf, "insert into stu2 values(\"%s\",\"%s\",\"登陆成功\",\"%d月  %d日  %02d:%02d\")",st.new_buf[0],\
            all.admin_all[1], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    if(falg==1){
        //管理员普通员工判断
        if(strncasecmp(result[12],"1",1)==0){
            usertypoe = 1;
        }else{
            usertypoe = 0;
        }
    }
    if(usertypoe==1){
    send(fsd,tips[16],128,0);}
    else{
    send(fsd,tips[17],128,0);}
    printf("工号为：< %s >\n姓名为:< %s >-----登录成功\n",all.admin_all[0],all.admin_all[1]);
    return usertypoe;
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
    //unsigned char (*p)[128]=all.admin_all;
    BUF_ZERO(buf);
    BUF_ZEROS(&st,1536);
    printf("BOOS 正在添加管理员信息\n");
    printf("正在验证BOOS添加权限密码\n");
    send(fsd, tips[28], 128, 0); //验证添加管理员权限密码
    if (recv(fsd, buf, sizeof(buf), 0) <= 0)
        return -1;
    if (strncasecmp(buf, PASSWORD, 6) == 0)
    {
        printf("密码验证成功\n");
        for (int i = 0; i < 11; i++)
        {
            //添加管理员信息
            send(fsd, tips[i], 128, 0);
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
        if (sqlite3_exec(db,buf,NULL,NULL,&errmsg) != SQLITE_OK)
        {
            send(fsd,tips[42],128,0);
            printf("工号已存在，注册管理员信息失败\n");
            return -1;
        }
    }
    else
    {
        printf("BOSS 权限验证未通过，密码错误\n");
        send(fsd, tips[33], 128, 0);
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
        send(fsd,tips[40],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    //printf("%ld\t%ld\n",strlen(&st.new_buf[0][0]),strlen(st.new_buf[1]));
    sprintf(all.admin_all[0], "%s", st.new_buf[0]); //保存登录用户工号信息
    sprintf(all.admin_all[1], "%s", st.new_buf[1]); //保存登录用户姓名信息
    printf("BOOS 添加管理员信息成功\n");
    return 0;
}

//添加信息
int sqlite3_add(sqlite3 *db, int fsd, struct ad_data id)
{
    char buf[128] = {"\0"};
    char out;
    int falg = 1;
    int usertypoe = 0;
    int row,column;
    char **result = NULL;
    char *errmsg = NULL;
    struct inifo st;
    time_t t = 0;
    struct tm *time_cat = NULL;
    BUF_ZEROS(&st,1536);
    row=column=0;
    for (int i = 0; i < 11; i++)
    {
        //添加基本员工员信息
        send(fsd, tips[i], 128, 0);
        if(recv(fsd, &st.new_buf[i][0], 128, 0)==0)
            return -1;
        printf("%s%s\n", tips[i], &st.new_buf[i][0]);
    }
    do{
        //判断是否需要修改工号
        BUF_ZERO(buf);
        sprintf(buf, "%s<%s>,%s", tips[11], &st.new_buf[0][0], tips[12]);
        send(fsd, buf, sizeof(buf), 0);
        if (recv(fsd, &out, 1, 0) == 0)
            return -1;
        //重新获取工号
        if (strncasecmp(&out, "n",1) == 0)
        {
            //清除工号信息
            BUF_ZEROS(&st.new_buf[0][0], 128);
            //重新获取工号
            send(fsd, tips[0], 128, 0);
            if (recv(fsd, &st.new_buf[0][0], sizeof(&st.new_buf[0][0]), 0) == 0)
                return -1;
        }else if(strncasecmp(&out,"y",1)==0){
            out = 0;
        }else{send(fsd,tips[35],128,0);}
    }while(out);
    do{
        //判断是否设置为管理员
        BUF_ZERO(buf);
        send(fsd, tips[13], 128, 0);
        if (recv(fsd, &out, 1, 0) == 0)
            return -1;
        if (strncasecmp(&out, "Y",1) == 0){
            usertypoe = 1;out=0;}
        else if(strncasecmp(&out,"n",1)==0){
            out = 0;
        }else{send(fsd,tips[35],128,0);}
    }while(out);

    //判断工号是否重复
    do
    {
         //判断工号是否重复
        BUF_ZERO(buf);
        sprintf(buf, "select * from stu1  where \"工号\"=\"%s\"", &st.new_buf[0][0]);
        if (sqlite3_get_table(db, buf, &result, &row,&column, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],128,0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        if((row&&column)!=0)
        {
            
            send(fsd, tips[34], 128, 0);
            if (recv(fsd, &out, 1, 0) == 0)
               return -1;
            if ((out == 'Y') || (out == 'y'))
            {
                //清除工号信息
                BUF_ZEROS(&st.new_buf[0][0], 128);
                //重新获取工号
                send(fsd, tips[0], 128, 0);
                if (recv(fsd, &st.new_buf[0][0], sizeof(&st.new_buf[0][0]), 0) == 0)
                    return -1;
            }
            else if(strcasecmp(&out,"n")==0){
                out = 0; //结束循环的条件
                sqlite3_free_table(result);
                return -1;
            }else{send(fsd,tips[35],128,0);}
        }else{
            out = 0; //结束循环条件
        }
    } while (out);
    //保存到数据库
    BUF_ZERO(buf);
    sprintf(buf, "insert into stu1 values(%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\
        \"%s\",\"%s\",\"%s\")",usertypoe, &st.new_buf[0][0], &st.new_buf[1][0],&st.new_buf[2][0],\
        &st.new_buf[3][0], &st.new_buf[4][0], &st.new_buf[5][0],&st.new_buf[6][0], &st.new_buf[7][0],\
        &st.new_buf[8][0], &st.new_buf[9][0], &st.new_buf[10][0]);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    //保存操作记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(buf);
    sprintf(buf, "insert into stu2 values(\"%s\",\"管理员:%s\",\"添加了:%s的员工\",\
        \"%d月  %d日  %02d:%02d\")",&id.admin_all[0][0], &id.admin_all[1][0],&st.new_buf[1][0],\
        time_cat->tm_mon + 1,time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, buf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
   
    send(fsd,tips[14],128,0);
    if(recv(fsd,&out,1,0)==0)
        return -1;
    do{
        if(strcasecmp(&out,"y")==0){
            sqlite3_add(db,fsd,id);out=0;}
        else if(strcasecmp(&out,"n")==0){
            out = 0;
        }else{
            send(fsd,"输入有误,请重新输入:",128,0);
        }
    }while(out);
    printf("管理员添加了工号为< %s >,姓名< %s >员工信息成功\n",st.new_buf[0],st.new_buf[1]);
    //释放查询结果
    sqlite3_free_table(result);
    return 0;
}

//删除信息
int sqlite3_del(sqlite3 *db, int fsd, struct ad_data id)
{
    char *errmsg = NULL;
    char **result = NULL;
    int row,column;
    char buf[10] = {"\0"};
    char ubuf[128] = {"\0"};
    time_t t = 0;
    struct tm *time_cat = NULL;
    //获取要删除的工号
    BUF_ZERO(buf);
    BUF_ZERO(ubuf);
    row=column=0;
    send(fsd, tips[15], 128, 0);
    if (recv(fsd, buf, sizeof(buf), 0) == 0)
        return -1;
    //判断工号是否存在
    sprintf(ubuf,"select * from stu1 where 工号=\"%s\"",buf);
    if(sqlite3_get_table(db,ubuf,&result,&row,&column,&errmsg)!=SQLITE_OK){
        send(fsd,tips[36],128,0);
        fprintf(stderr,"%s:__%d__,sqlite3_exet:%s\n",__FILE__,__LINE__,errmsg);
        return -1;
    }
    if(row&&column){
        //执行删除语句
        BUF_ZERO(ubuf);
        sprintf(ubuf, "delete from stu1 where \"工号\"=\"%s\"", buf);
        if (sqlite3_exec(db, ubuf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],128,0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        //删除成功
        send(fsd, tips[25], 128, 0);
        buf[strlen(buf)]='\n';
        send(fsd,buf,128,0);
        buf[strlen(buf)-1]='\0';
        //保存操作历史记录
        time(&t);                 //获取时间
        time_cat = localtime(&t); //转换时间
        BUF_ZERO(ubuf);
        sprintf(ubuf, "insert into stu2 values(\"%s\",\"管理员%s\",\"管理员删除了工号为：%s的员工\",\"%d月  %d日  %02d:%02d\")",&id.admin_all[0][0],&id.admin_all[1][0],\
            buf, time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
        if (sqlite3_exec(db, ubuf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],128,0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        
        printf("管理员< %s >删除工号< %s >员工信息成功\n",&id.admin_all[1][0],buf);
    }else{send(fsd,tips[24],128,0);}
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
    int row,column;
    char **result=NULL;
    char *errmsg = NULL;
    char *p=NULL;
    //初始化数组
    BUF_ZERO(nbuf);
    BUF_ZEROS(buf, 256);
    row=column=0;
    //判断是管理员修改还是普通员工修改
    if (usertypoe == 1)
    {
        //管理员修改程序
         send(fsd, tips[21], 128, 0); //获取工号
        if (recv(fsd, &buf[0][0], 128, 0) == 0)
        return -1;
        //工号匹配
    sprintf(nbuf,"select * from stu1 where 工号=\"%s\"",&buf[0][0]);
   if (sqlite3_get_table(db,nbuf, &result, &row, &column, &errmsg) != SQLITE_OK)     //获取工号
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    if((row&&column)==0){
        printf("没有< %s >该用户信息\n",&buf[0][0]);
        send(fsd,tips[41],128,0);     //暂无用户信息
        return -1;
    }else{
        send(fsd, tips[(menu_int[falg-1])], 128, 0);
        if (recv(fsd, &buf[1][0], 128, 0) == 0)
            return -1;
        sprintf(nbuf, "update stu1 set \"%s\"=\"%s\" where 工号=\"%s\"", menu[falg-1], &buf[1][0], &buf[0][0]);
        if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],128,0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        p=menu[falg-1];
        send(fsd, tips[22], 128, 0);
    }
    }
    else if (usertypoe == 0)
    {
        //员工修改程序
        send(fsd, tips[(menu_two_int[falg-1])], 128, 0);
        if (recv(fsd, &buf[1][0], 128, 0) == 0)
            return -1;
        sprintf(nbuf, "update stu1 set \"%s\"=\"%s\" where 工号=\"%s\"", menu_two[falg-1], &buf[1][0], &id.admin_all[0][0]);
        if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[38],128,0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
        p=menu_two[falg-1];
        send(fsd, tips[22], 128, 0);
    }
    //保存记录
    time(&t);                 //获取时间
    time_cat = localtime(&t); //转换时间
    BUF_ZERO(nbuf);
    //管理员数据修改记录
   if(usertypoe==1){
    printf("管理员< %s >修改了< %s的 : %s >为< %s >\n",&id.admin_all[1][0],buf[0],p,&buf[1][0]);
    sprintf(nbuf, "insert into stu2 values(\"%s\",\"%s\",\"管理员修改了用户< %s >%s为%s\",\"%d月  %d日  %02d:%02d\")", &id.admin_all[0][0],\
         &id.admin_all[1][0],buf[0],p,&buf[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    }else{      //用户修改记录
    sprintf(nbuf, "insert into stu2 values(\"%s\",\"%s\",\"修改%s为%s\",\"%d月  %d日  %02d:%02d\")", &id.admin_all[0][0], &id.admin_all[1][0],p,\
            &buf[1][0], time_cat->tm_mon + 1, time_cat->tm_mday,time_cat->tm_hour, time_cat->tm_min);
    if (sqlite3_exec(db, nbuf, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        send(fsd,tips[38],128,0);
        fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
        return -1;
    }
    printf("用户< %s >修改了< %s >为< %s >\n",&id.admin_all[1][0],p,&buf[1][0]);
    }
    //释放查询结果
    sqlite3_free_table(result);
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
    row=column=0;
    BUF_ZERO(sql);
    BUF_ZERO(buf);
    //判断是管理员还是普通员工
    if (usertypoe == 1)
    {
        //管理员查找程序（按姓名查询员工信息）
        if (falg == 1)
        {
            send(fsd, tips[1], 128, 0);
            if(recv(fsd,buf,sizeof(buf),0)==0)
                return -1;
            sprintf(sql,"select * from stu1 where 姓名=\"%s\"",buf);
            if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
            {
                send(fsd,tips[36],128,0);
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return -1;
            }
           
        }
        //管理员查找程序（全部员工信息查询）
        else if(falg == 2){
            sprintf(sql,"select * from stu1");
            if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
            {
                send(fsd,tips[36],128,0);
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return -1;
            }
        }
        //管理员查找程序（历史记录查询）
        else if(falg == 5){
            sprintf(sql,"select * from stu2");
            if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
            {
                send(fsd,tips[36],128,0);
                fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
                return -1;
            }
        }
        else{send(fsd,tips[35],128,0);return -1;}

    }
    //员工查找程序
    else if (usertypoe == 0)
    {
        sprintf(sql,"select * from stu1 where 工号=\"%s\"",&id.admin_all[0][0]);
        //员工查找程序只能查找自己的
        if (sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) != SQLITE_OK)
        {
            send(fsd,tips[36],128,0);
            fprintf(stderr, "%s:__%d__,sqlite3_exet:%s\n", __FILE__, __LINE__, errmsg);
            return -1;
        }
    }
     BUF_ZERO(buf);
    //发送数据
    if(row&&column)
    {
        if(falg!=5){
        BUF_INIT(buf,'#',110);
        buf[strlen(buf)-1]='\n';
        send(fsd,buf,128,0);
        BUF_ZERO(buf);
        //优化菜单
        sprintf(buf,"%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s\n",result[(index)],9,result[(index+1)],\
            9,result[(index+2)],9,result[(index+3)],9,result[(index+4)],9,result[(index+5)],9,result[(index+6)],9,result[(index+7)],\
            9,result[(index+8)],9,result[(index+9)],9,result[(index+10)],9,result[(index+11)]);
            send(fsd,buf,sizeof(buf),0);
            BUF_ZERO(buf);
            index+=12;
        //员工信息处理流程
        for(int i=0;i<row;i++){
            sprintf(buf,"%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s%c,%s\n",result[(index)],32,result[(index+1)],\
            32,result[(index+2)],32,result[(index+3)],32,result[(index+4)],32,result[(index+5)],32,result[(index+6)],32,result[(index+7)],\
            32,result[(index+8)],32,result[(index+9)],32,result[(index+10)],32,result[(index+11)]);
            send(fsd,buf,sizeof(buf),0);
            BUF_ZERO(buf);
            index = index+12;
        }
        BUF_INIT(buf,'#',110);
        buf[strlen(buf)-1]='\n';
        send(fsd,buf,128,0);
        }else{
            //优化菜单
            BUF_INIT(buf,'#',70);
            buf[strlen(buf)-1]='\n';
            send(fsd,buf,128,0);
            BUF_ZERO(buf);
            sprintf(buf,"%s%c%s%c%s%c%s\n",result[(index)],9,result[(index+1)],9,result[(index+2)],9,result[(index+3)]);
            send(fsd,buf,sizeof(buf),0);
            BUF_ZERO(buf);
            index = index+4;
            //历史记录处理流程
            for(int i=0;i<row;i++){
            sprintf(buf,"%s%c,%s%c,%s%c,%s\n",result[(index)],9,result[(index+1)],9,result[(index+2)],9,result[(index+3)]);
            send(fsd,buf,sizeof(buf),0);
            BUF_ZERO(buf);
            index = index+4;
            }
            //优化菜单
            BUF_INIT(buf,'#',70);
            buf[strlen(buf)-1]='\n';
            send(fsd,buf,128,0);
        }
    }else{send(fsd,tips[41],128,0);
            printf("信息查询失败\n");
            return -1;
        }
    if(falg==5){
        send(fsd,tips[26],128,0);
        printf("历史记录信息查询成功\n");}
    else{
        send(fsd,tips[37],128,0);
        printf("员工信息查询成功\n");}
    
    //释放查询结果
    sqlite3_free_table(result);
    return 0;
}
/*********************************************************/
