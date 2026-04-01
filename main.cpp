#include <opencv2/opencv.hpp>
#include "image.h"
#include <string>
#include <format>
#include <iostream>

// 使用现代的OpenCV命名空间
using namespace cv;

// 颜色定义
Scalar col_white, col_black, col_blue, col_red, col_green, col_gray, col_purple;

// 配置参数
#define change_speed 2
#define NUMOFSTART 0
#define NUMOFEND 4073

// 图像变量 - 使用Mat替代IplImage*
Mat pImg_1;          // 原图
Mat pImg_2;          // 当前图（带界面）
std::string path;
int have_paint_midline = 0, pic_num = 0;

uint8_t Buffer[CAMERA_H * CAMERA_W]; // 图像数据缓冲区
uint8_t* fullBuffer = Buffer;


// 函数声明
void col_init(void);
void create_window(void);
void create_pic_now_background(void);
void load_pic(void);
void show_pic(void);
void on_mouse(int event, int x, int y, int flags, void* userdata);
void find_midline(void);

// 颜色初始化
void col_init(void) {
    col_white = Scalar(255, 255, 255);
    col_black = Scalar(0, 0, 0);
    col_blue = Scalar(255, 0, 0);      // BGR顺序
    col_green = Scalar(0, 255, 0);
    col_purple = Scalar(255, 0, 255);
    col_red = Scalar(0, 0, 255);
    col_gray = Scalar(100, 100, 100);
}

// 创建窗口
void create_window(void) {
    namedWindow("原图", WINDOW_NORMAL);
    namedWindow("当前图", WINDOW_NORMAL);
}

// 创建带界面的背景图
void create_pic_now_background(void) {
    int width = CAMERA_W + 80;
    int height = CAMERA_H + 80;

    // 创建白色背景的彩色图像
    pImg_2 = Mat(height, width, CV_8UC3, col_white);

    // 绘制按钮
    rectangle(pImg_2, Point(CAMERA_W + 10, 5), Point(CAMERA_W + 60, 20), col_black, -1);
    putText(pImg_2, "start", Point(CAMERA_W + 10, 15), FONT_HERSHEY_SIMPLEX, 0.4, col_blue, 1);

    rectangle(pImg_2, Point(CAMERA_W + 10, 25), Point(CAMERA_W + 60, 40), col_black, -1);
    putText(pImg_2, "midline", Point(CAMERA_W + 10, 35), FONT_HERSHEY_SIMPLEX, 0.4, col_blue, 1);

    rectangle(pImg_2, Point(CAMERA_W + 10, 45), Point(CAMERA_W + 60, 60), col_black, -1);
    putText(pImg_2, "turn_to", Point(CAMERA_W + 10, 55), FONT_HERSHEY_SIMPLEX, 0.4, col_blue, 1);

    rectangle(pImg_2, Point(CAMERA_W + 10, 65), Point(CAMERA_W + 60, 80), col_black, -1);
    putText(pImg_2, "draw", Point(CAMERA_W + 10, 75), FONT_HERSHEY_SIMPLEX, 0.4, col_blue, 1);

    rectangle(pImg_2, Point(CAMERA_W + 10, 85), Point(CAMERA_W + 60, 100), col_black, -1);
    putText(pImg_2, "restart", Point(CAMERA_W + 10, 95), FONT_HERSHEY_SIMPLEX, 0.4, col_blue, 1);
}

// 加载图片
void load_pic(void) {

    path = std::format("D:/SmartcarImage/pictures/front_stairs/Image{}.jpg", pic_num + 120);

    // 使用imread替代cvLoadImage，0表示灰度加载
    pImg_1 = imread(path, IMREAD_GRAYSCALE);
    if (pImg_1.empty()) {
        printf("can not load picture!\n");
        return;
    }

    imshow("原图", pImg_1);

    // 将图像数据复制到buffer中
    memcpy(fullBuffer, pImg_1.data, CAMERA_H * CAMERA_W * sizeof(uint8_t));
    /*else {
        for (int i = 0; i < CAMERA_H; ++i) {
            memcpy(&image_Buffer_0[i][0], pImg_1.ptr<uint8_t>(i), CAMERA_W * sizeof(uint8_t));
        }
    }*/

    // 调用图像处理函数
    if (find_midline)
        image_main();
}

// 显示图片
void show_pic(void) {
    // 重新创建显示图像
    //create_pic_now_background();

    // 将处理后的图像(IMG数组)绘制到pImg_2的对应区域
    for (int i = 0; i < CAMERA_H; i++) {
        for (int j = 0; j < CAMERA_W; j++) {
            Scalar color;

            // 根据颜色值设置颜色
            if (j < LEFT_SIDE || j > RIGHT_SIDE) {
                color = col_gray;
            }
            else {
                switch (IMG[i][j]) {
                case white: color = col_white; break;
                case black: color = col_black; break;
                case blue: color = col_blue; break;
                case green: color = col_green; break;
                case red: color = col_red; break;
                case purple: color = col_purple; break;
                case gray: color = col_gray; break;
                default: color = col_gray; break;
                }
            }

            // 在图像上绘制像素点
            pImg_2.at<Vec3b>(i, j) = Vec3b(
                static_cast<uchar>(color[0]),
                static_cast<uchar>(color[1]),
                static_cast<uchar>(color[2])
            );
        }
    }

    // 显示图片编号
    rectangle(pImg_2, Point(0, CAMERA_H + 1), Point(100, CAMERA_H + 21), col_white, -1);
    putText(pImg_2, std::format("picture {0}", pic_num), 
        Point(5, CAMERA_H + 20), FONT_HERSHEY_SIMPLEX, 0.4, col_red, 1);

    // 显示图像
    imshow("当前图", pImg_2);
}

// 鼠标事件回调函数
void on_mouse(int event, int x, int y, int flags, void* userdata) {
    static int start_flag = 0;
    static int restart_flag = 0;
    static int turnto_flag = 0;
    static int draw_flag = 0;
    static int rightbutton_flag = 0;
    static int fre_num = 0;
    static int midline_flag = 0;

    char my_text[60];

    if (draw_flag == 1) {
        switch (event) {
        case EVENT_LBUTTONDOWN: {
            if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 65 && y < 80) {
                draw_flag = 0;
            }
            else {
                load_pic();
            }
            break;
        }
        case EVENT_RBUTTONUP: {
            rightbutton_flag = 0;
            break;
        }
        case EVENT_RBUTTONDOWN: {
            rightbutton_flag = 1;
            break;
        }
        case EVENT_MOUSEMOVE: {
            // 显示坐标
            rectangle(pImg_2, Point(0, CAMERA_H + 22), Point(130, CAMERA_H + 50), col_white, -1);
            
            putText(pImg_2, std::format("x = {0} y = {1}", x, y), Point(10, CAMERA_H + 40), FONT_HERSHEY_SIMPLEX, 0.4, col_blue, 1);

            if (x >= CAMERA_W || y >= CAMERA_H) {
                break;
            }
            break;
        }
        }
    }
    else if (start_flag == 0) {
        switch (event) {
        case EVENT_LBUTTONDOWN: {
            if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 5 && y < 20) {
                start_flag = 1;
            }
            else if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 25 && y < 40) {
                midline_flag = midline_flag == 0 ? 1 : 0;
            }
            else if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 45 && y < 60) {
                printf("Please input the num of picture you want to check:");
				std::cin >> pic_num;
                load_pic();
            }
            else if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 65 && y < 80) {
                draw_flag = 1;
            }
            else if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 85 && y < 100) {
                pic_num = 1;
                load_pic();
            }
            else {
                pic_num++;
                if (pic_num >= NUMOFEND) pic_num = NUMOFSTART;
                load_pic();
            }
            break;
        }
        case EVENT_RBUTTONDOWN: {
            if (pic_num <= NUMOFSTART) pic_num = NUMOFEND;
            pic_num--;
            load_pic();
            break;
        }
        case EVENT_MBUTTONDOWN: {
            std::cout << std::format("pt.x = {0}, pt.y = {1}\n", x, y);
            rectangle(pImg_2, Point(0, CAMERA_H + 20), Point(80, CAMERA_H + 40), col_white, -1);
            putText(pImg_2, std::format("({0},{1})", x, y), Point(0, CAMERA_H + 40), FONT_HERSHEY_SIMPLEX, 0.4, col_red, 1);
            IMG[y][x] = 80;
            break;
        }
        }
    }
    else if (start_flag == 1) {
        if (event == EVENT_LBUTTONDOWN) {
            if (x > CAMERA_W + 10 && x < CAMERA_W + 60 && y > 5 && y < 20) {
                start_flag = 0;
            }
        }
        else {
            fre_num++;
            if (fre_num == change_speed) {
                pic_num++;
                if (pic_num == NUMOFEND) {
                    pic_num = NUMOFSTART;
                }
                load_pic();
                fre_num = 0;
            }
        }
    }

    if (midline_flag == 1 && !have_paint_midline) {
        have_paint_midline = 1;
    }

    show_pic();
}

// 中线检测函数
void find_midline(void) {
    // 调用图像处理主函数
    image_main();
}

// 主函数
int main() {
    col_init();
    create_window();
    create_pic_now_background();

    pic_num = NUMOFSTART;
    load_pic();
    show_pic();

    // 设置鼠标回调
    setMouseCallback("当前图", on_mouse, nullptr);

    while (true) {
        int key = waitKey(0);

        if (key == 'a') {
            pic_num--;
            if (pic_num == NUMOFSTART - 1) {
                pic_num = NUMOFEND;
            }
            load_pic();

            show_pic();
        }

        if (key == 'd') {
            pic_num++;
            if (pic_num == NUMOFEND + 1) {
                pic_num = NUMOFSTART;
            }
            load_pic();

            show_pic();
        }

        if (key == 27) {  // ESC键退出
            break;
        }
    }

    destroyAllWindows();
    return 0;
}