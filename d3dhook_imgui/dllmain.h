using namespace std;
#pragma once
//定义导出、导入宏
#ifdef D3DHOOKKIERO_EXPORTS
#define CPPDLLDEMO_API __declspec(dllexport)
#else
#define CPPDLLDEMO_API __declspec(dllimport)
#endif // CPPDLLDEMO_EXPORTS

//头文件标识
#ifndef CPPDLLDEMO_H
#define CPPDLLDEMO_H

/*----------------------{
包含一些不得不包含的头文件
}----------------------*/

/*----------------------{
这里写一些结构体、类等
}----------------------*/


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


	CPPDLLDEMO_API void SayHello();


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !CPPDLLDEMO_H