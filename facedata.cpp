#include "StdAfx.h"
#include "facedata.h"


facedata::facedata(void)
{
}


facedata::~facedata(void)
{
}
double facedata::getSum()
{
	return k1+k2+k3+k4;
}
void facedata::set_k1(double val)
{
	k1=val;
}
void facedata::set_k2(double val)
{
	k2=val;
}
void facedata::set_k3(double val)
{
	k3=val;
}
void facedata::set_k4(double val)
{
	k4=val;
}
void facedata::set_pic_name(string str)
{
	picture_name=str;
}
double facedata::get_k1()
{
	return k1;
}
double facedata::get_k2()
{
	return k2;
}
double facedata::get_k3()
{
	return k3;
}
double facedata::get_k4()
{
	return k4;
}
void facedata::get_pic_name(string &str)
{
	str=picture_name;
}