//
// Created by xingkong on 2017/4/24.
//

#ifndef ANDROID_NDK_USB_TEST_MAIN_H
#define ANDROID_NDK_USB_TEST_MAIN_H

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <android/log.h>

#include "d2cc.h"

pthread_t *ReadThreadmain;
pthread_t *JudgeThreadmain;

D2cc d2cc;

int endPointIn1;
int endPointOut1;
int fdesc;
int count=0;
bool begin=0;

#define DATA_PATH "/storage/sdcard0/PhasedArray/usbdat.dat"

int actual_read_main=0;
long allnum=0;
//unsigned char last_byte=0;
//int errors=0;
//bool begin=0;

unsigned char * read_buffer_ptr;  //USB读写缓存
//unsigned char * judge_buffer;  //USB读写缓存
unsigned char buff1[4096*5];
//unsigned char buff2[LENGTH];
//unsigned char judge_data[LENGTH];

unsigned char out_buffer[4];
//unsigned char * out_ptr;
int ffd;

//解包参数
int extract_length=0;
int packages=0;
unsigned char last_index=0;
int lost=0;

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

//void judge_error(int actual_read);
//void  * ReadDeviceFun(void *pArguments);
//void  * JudgeThreadFun(void *pArguments);
bool isreading=1;



#endif //ANDROID_NDK_USB_TEST_MAIN_H
