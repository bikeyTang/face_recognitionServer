#pragma once

#include <iostream>
#include <string>
using namespace std;

class facedata
{
private:
	double k1,k2,k3,k4;
	string picture_name;
public:
	facedata(void);
	~facedata(void);
	
	double getSum();
	void set_k1(double val);
	void set_k2(double val);
	void set_k3(double val);
	void set_k4(double val);
	double get_k1();
	double get_k2();
	double get_k3();
	double get_k4();
	void get_pic_name(string &str);
	void set_pic_name(string str);
};

