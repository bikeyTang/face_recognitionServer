// recognitionServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <WINSOCK2.H>
#include <stdio.h>
#include <iostream>
#include <windows.h>
#include "mysql.h"
#include "winsock.h"
#include <string>
#include "facedata.h"
#include <vector>
#include <iomanip>
#include <map>
#include <math.h>
#include <io.h>
#include "cv.h"
#include <iostream>
#include <fstream>
using namespace std;
#pragma comment(lib,"libmysql.lib")
#pragma comment(lib,"WS2_32.lib")
#define BUF_SIZE	64
typedef struct Face{
	double k1;
	double k2;
	double k3;
	double k4;
};
bool startService();
void recognition(facedata fd,vector<string> &vs);
void mapper(facedata fd,vector<map<double,string> > &vm);
void getResult(string sql,facedata fd,map<double,string> &m);
void reducer(vector<string> &vs,vector<map<double,string>> vm);
void getSimilar(vector<facedata> vf,facedata fd,map<double,string> &m);
bool getConn();
void query(string sql,vector<facedata> &vf);
void findSimilar(vector<facedata> vf,facedata fd,vector<string> &vs);
void splitStr(string s,string delim,vector<string> &result);
double correlationCoeff(facedata rx,facedata ry);
template<class Out_Type, class In_Type> Out_Type type_convert(const In_Type& T);
MYSQL m_sqlCon;
MYSQL_RES *res;
MYSQL_ROW row;
int _tmain(int argc, _TCHAR* argv[])
{
	if(!getConn())
	{
		cout<<"connect mysql failed!"<<endl;
		return 0;
	}
	startService();
	mysql_close(&m_sqlCon);
	system("pause");
	return 0;
}
/*
	服务器端等待连接
*/
bool startService()
{
	WSADATA	wsd;
	SOCKET	s;
	int		nRet;
	Face fs;
	facedata fd;
	vector<string> vs;
	// 初始化套接字动态库
	if(WSAStartup(MAKEWORD(2,2),&wsd) != 0)
	{
		printf("WSAStartup failed !\n");
		return false;
	}

	// 创建套接字
	s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(s == INVALID_SOCKET)
	{
		printf("socket() failed ,Error Code:%d\n",WSAGetLastError());
		WSACleanup();
		return false;
	}
	SOCKADDR_IN addrSrv;
	SOCKADDR_IN addrClient;
	char		buf[BUF_SIZE];
	int			len = sizeof(SOCKADDR);

	// 设置服务器地址
	ZeroMemory(buf,BUF_SIZE);
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(5000);

	// 绑定套接字
	nRet = bind(s,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	if(SOCKET_ERROR == nRet)   
    {   
        printf("bind failed !\n");   
        closesocket(s);   
        WSACleanup();   
        return false;   
    }
	 if (listen(s, SOMAXCONN) == SOCKET_ERROR)
		 wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
	wprintf(L"Listening on socket...\n");
    wprintf(L"Waiting for client to connect...\n");
	SOCKET sockConn=accept(s,(SOCKADDR*)&addrClient,&len);
	while(true)
	{
		// 从客户端接收数据
		//nRet = recvfrom(socketSrv,buf,BUF_SIZE,0,(SOCKADDR*)&addrClient,&len);
		nRet=recv(sockConn,buf,BUF_SIZE,0);
		if(SOCKET_ERROR == nRet)   
		{   
			//printf("recvfrom failed !\n");   
			//closesocket(s);   
			//WSACleanup();   
			//return false;   
		}else{
			memcpy(&fs,buf,BUF_SIZE);
			fd.set_k1(fs.k1);
			fd.set_k2(fs.k2);
			fd.set_k3(fs.k3);
			fd.set_k4(fs.k4);
			recognition(fd,vs);
			int count=0;
			DWORD dwNumberOfBytesRead;
			vector<string>::iterator it=vs.begin();
			while(it!=vs.end())
			{
				unsigned long long file_size = 0;
				//因文件大小为6*3209，所以分6次传送，每次大小为3209
				char Buffer[3209];
				string path=*it;
				HANDLE fhandle = CreateFile((LPCSTR)path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
				file_size = GetFileSize(fhandle,NULL);
				 do 
				{
					ReadFile(fhandle,Buffer,sizeof(Buffer),&dwNumberOfBytesRead,NULL);
					send(sockConn,Buffer,dwNumberOfBytesRead,0);
				} while (dwNumberOfBytesRead);
				cout<<(++count)<<endl;
				dwNumberOfBytesRead=0;
				CloseHandle(fhandle);
				it++;
			}
			vs.clear();
		}
	}
	closesocket(s);
	WSACleanup();
	return true;
}
/*
	vs用来存放最后得到的最相似对象的路径
*/
void recognition(facedata fd,vector<string> &vs)
{
	vector<map<double,string> > vm;
	//把待识别对象映射到对应表中查询
	mapper(fd,vm);
	//对mapper后的结果进行规约
	reducer(vs,vm);
}
/*
	根据对象的hash值，找到所映射到的表
*/
void mapper(facedata fd,vector<map<double,string> > &vm)
{
	double hash_value=fd.getSum();
	string first=type_convert<string>(hash_value).substr(0,1);
	int key=type_convert<int>(first);
	string sql="select k1,k2,k3,k4,picture_name from facebase";
	if(key==0)
	{
		string mysql=sql;
		map<double,string> m0,m1;
		getResult(mysql.append(type_convert<string>(key)),fd,m0);
		vm.push_back(m0);
		mysql=sql;
		getResult(mysql.append(type_convert<string>(key+1)),fd,m1);
		vm.push_back(m1);
	}else if(key==9)
	{
		string mysql=sql;
		map<double,string> m8,m9;
		getResult(mysql.append(type_convert<string>(key-1)),fd,m8);
		vm.push_back(m8);
		mysql=sql;
		getResult(mysql.append(type_convert<string>(key)),fd,m9);
		vm.push_back(m9);
	}else if(key==1)
	{
		string mysql=sql;
		map<double,string> m0,m1,m2;
		getResult(mysql.append(type_convert<string>(9)),fd,m0);
		vm.push_back(m0);
		mysql=sql;
		getResult(mysql.append(type_convert<string>(key)),fd,m1);
		vm.push_back(m1);
		mysql=sql;
		getResult(mysql.append(type_convert<string>(key+1)),fd,m2);
		vm.push_back(m2);
	}
	else
	{
		string mysql=sql;
		map<double,string> m0,m1,m2;
		getResult(mysql.append(type_convert<string>(key-1)),fd,m0);
		vm.push_back(m0);
		mysql=sql;
		getResult(mysql.append(type_convert<string>(key)),fd,m1);
		vm.push_back(m1);
		mysql=sql;
		getResult(mysql.append(type_convert<string>(key+1)),fd,m2);
		vm.push_back(m2);
	}
}
/*
	从对应表中查询数据并找到相似结果保存在map中
*/
void getResult(string sql,facedata fd,map<double,string> &m)
{
	vector<facedata> vf;
	query(sql,vf);
	getSimilar(vf,fd,m);
}
/*
	对map后计算的结果进行规约
*/
void reducer(vector<string> &vs,vector<map<double,string>> vm)
{
	map<double,string> remap;
	//把vector的map中的对象插入到新的map中。
	if(vm.size()!=0)
	{
		vector<map<double,string> >::iterator it=vm.begin();
		while(it!=vm.end())
		{
			map<double,string>::iterator mit=(*it).begin();
			while(mit!=(*it).end())
			{
				remap.insert(make_pair((*mit).first,(*mit).second));
				mit++;
			}
			it++;
		}
	}
	//得到的对象多于6个时只取前6个，否则全取。
	if(remap.size()!=0)
	{
		map<double,string>::iterator remit=remap.begin();
		if(remap.size()<6)
		{
			while(remit!=remap.end())
			{
				vs.push_back((*remit).second);
				remit++;
			}
		}else
		{
			for(int i=0;i<6;i++)
			{
				vs.push_back((*remit).second);
				remit++;
			}
		}
	}
	
}
/*
	连接mysql数据库
*/
bool getConn()
{
	mysql_init(&m_sqlCon);
	// localhost:服务器 root为账号密码 test为数据库名 3306为端口
	if(!mysql_real_connect(&m_sqlCon, "localhost","root","mysql","test527",3306,NULL,0))
	{
		//AfxMessageBox(_T("数据库连接失败!"));
		cout<<"数据库连接失败!"<<endl;
		return false;
	}
	cout<<"数据库连接成功！"<<endl;
	return true;
}
/*
	根据sql语句，查询表中数据。
*/
void query(string sql,vector<facedata> &vf)
{
	int result=mysql_query(&m_sqlCon,sql.c_str());
	if(!result)
	{
		res=mysql_store_result(&m_sqlCon);
		if(res)
		{
			int field_num=mysql_num_fields(res);
			while(row=mysql_fetch_row(res))
			{
				facedata fd;
				fd.set_k1(atof(row[0]));
				fd.set_k2(atof(row[1]));
				fd.set_k3(atof(row[2]));
				fd.set_k4(atof(row[3]));
				fd.set_pic_name(row[4]);
				vf.push_back(fd);
			}
		}
		if(res!=NULL)
			mysql_free_result(res);
	}
	
}
/*
	利用欧式距离公式找到前几个相似对象
*/
void getSimilar(vector<facedata> vf,facedata fd,map<double,string> &m)
{
	string pic_name="";
	facedata ft;
	map<double,string> sort_map;
	double k1sq,k2sq,k3sq,k4sq;
	string picture="";
	fd.get_pic_name(picture);
	for(int i=0;i<vf.size();i++)
	{
		k1sq=fd.get_k1()-vf[i].get_k1();
		k2sq=fd.get_k2()-vf[i].get_k2();
		k3sq=fd.get_k3()-vf[i].get_k3();
		k4sq=fd.get_k4()-vf[i].get_k4();
		double value=k1sq*k1sq+k2sq*k2sq+k3sq*k3sq+k4sq*k4sq;
		vf[i].get_pic_name(pic_name);
		sort_map.insert(make_pair(value,pic_name));
	}
	map<double,string>::iterator it=sort_map.begin();
	if(sort_map.size()>=5){
		for(int i=0;i<5;i++)
		{
			m.insert(make_pair((*it).first,(*it).second));
			//cout<<pic_name<<endl;
			it++;
			
		}
	}else
	{
		while(it!=sort_map.end())
		{
			m.insert(make_pair((*it).first,(*it).second));
			//cout<<pic_name<<endl;
			it++;
		}
	}
}
/*
	找相似
*/
void findSimilar(vector<facedata> vf,facedata fd,vector<string> &vs)
{
	
	string pic_name="";
	facedata ft;
	map<double,facedata> sort_map;
	double k1sq,k2sq,k3sq,k4sq;
	string picture="";
	fd.get_pic_name(picture);
	for(int i=0;i<vf.size();i++)
	{
		k1sq=fd.get_k1()-vf[i].get_k1();
		k2sq=fd.get_k2()-vf[i].get_k2();
		k3sq=fd.get_k3()-vf[i].get_k3();
		k4sq=fd.get_k4()-vf[i].get_k4();
		double value=k1sq*k1sq+k2sq*k2sq+k3sq*k3sq+k4sq*k4sq;

		sort_map.insert(make_pair(value,vf[i]));
	}
	map<double,facedata>::iterator it=sort_map.begin();
	if(sort_map.size()>=5){
		for(int i=0;i<5;i++)
		{
			(*it).second.get_pic_name(pic_name);
			vs.push_back(pic_name);
			//cout<<pic_name<<endl;
			it++;
			
		}
	}else
	{
		while(it!=sort_map.end())
		{
			(*it).second.get_pic_name(pic_name);
			vs.push_back(pic_name);
			//cout<<pic_name<<endl;
			it++;
		}
	}
}
/*
	求两个对象聚类后两个对象的相关性
*/
double correlationCoeff(facedata rx,facedata ry)
{
	double means_x=rx.getSum()/4;
	double means_y=ry.getSum()/4;
	double std_x=0,std_y=0,px=0,py=0;
	double zx[4],zy[4],x[4],y[4],sumr=0,corr=0;
	x[0]=rx.get_k1();
	x[1]=rx.get_k2();
	x[2]=rx.get_k3();
	x[3]=rx.get_k4();
	y[0]=ry.get_k1();
	y[1]=ry.get_k2();
	y[2]=ry.get_k3();
	y[3]=ry.get_k4();
	for(int i=0;i<4;i++)
	{
		px+=pow(x[i]-means_x,2.0);
		py+=pow(y[i]-means_y,2.0);
	}
	
	std_x=sqrt(px/3);
	std_y=sqrt(py/3);
	for(int i=0;i<4;i++)
	{
		zx[i]=(x[i]-means_x)/std_x;
		zy[i]=(y[i]-means_y)/std_y;
		sumr+=zx[i]*zy[i];
	}
	corr=sumr/3;
	return corr;
}
/*

	根据分字符对字符串进行分块
*/
void splitStr(string s,string delim,vector<string> &result)
{
	char* p;
	p=strtok(const_cast<char*>(s.c_str()),delim.c_str());
	while(p)
	{
		result.push_back(p);
		p=strtok(NULL,delim.c_str());
	}
}
/*
	类型转化函数，把常见的int,double->string或string->double,int...
*/
template<class Out_Type, class In_Type> Out_Type type_convert(const In_Type& T)
{
	stringstream ss;
	ss<<T;
	Out_Type result;
	ss>>result;
	return result;
}

