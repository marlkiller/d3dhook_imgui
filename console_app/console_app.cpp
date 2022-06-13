// console_app.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#include "../d3dhook_imgui/dllmain.h"  //dll的头文件

int main()
{
    SayHello();
    std::cout << "Hello Console App!\n";
    cin.get();

}

