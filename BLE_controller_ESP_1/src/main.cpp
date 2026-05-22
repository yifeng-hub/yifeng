
/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include "ui_helpers.h"
//#include <Arduino.h>

#include <SPI.h>
#include "FS.h"

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 as the examples and demos are now part of the main LVGL library. */

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 320;



lv_indev_t * indev_keypad;
lv_group_t * group3;
lv_group_t * group2;
lv_group_t * group;
//lv_group_t * msgbox_group;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
#define CALIBRATION_FILE "/TouchCalData3"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

bool SwitchOn = false;

// Comment out to stop drawing black spots
#define BLACK_SPOT



#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    S
    intf(buf);
    Serial.flush();
}
#endif

// void touch_calibrate()
// {
//   uint16_t calData[5];
//   uint8_t calDataOK = 0;

//   // check file system exists
//   if (!SPIFFS.begin()) {
//     Serial.println("Formatting file system");
//     SPIFFS.format();
//     SPIFFS.begin();
//   }

//   // check if calibration file exists and size is correct
//   if (SPIFFS.exists(CALIBRATION_FILE)) {
//     if (REPEAT_CAL)
//     {
//       // Delete if we want to re-calibrate
//       SPIFFS.remove(CALIBRATION_FILE);
//     }
//     else
//     {
//       fs::File f = SPIFFS.open(CALIBRATION_FILE, "r");
//       if (f) {
//         if (f.readBytes((char *)calData, 14) == 14)
//           calDataOK = 1;
//         f.close();
//       }
//     }
//   }

//   if (calDataOK && !REPEAT_CAL) {
//     // calibration data valid
//     tft.setTouch(calData);
//   } else {
//     // data not valid so recalibrate
//     tft.fillScreen(TFT_BLACK);
//     tft.setCursor(20, 0);
//     tft.setTextFont(2);
//     tft.setTextSize(1);
//     tft.setTextColor(TFT_WHITE, TFT_BLACK);

//     tft.println("Touch corners as indicated");

//     tft.setTextFont(1);
//     tft.println();

//     if (REPEAT_CAL) {
//       tft.setTextColor(TFT_RED, TFT_BLACK);
//       tft.println("Set REPEAT_CAL to false to stop this running again!");
//     }

//     tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

//     tft.setTextColor(TFT_GREEN, TFT_BLACK);
//     tft.println("Calibration complete!");

//     // store data
//     fs::File f = SPIFFS.open(CALIBRATION_FILE, "w");
//     if (f) {
//       f.write((const unsigned char *)calData, 14);
//       f.close();
//     }
//   }
// }



/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp_drv );
}

/*Read the touchpad*/
// void my_touchpad_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
// {
//     uint16_t touchX, touchY;

//     bool touched = tft.getTouch( &touchX, &touchY, 600 );

//     if( !touched )
//     {
//         data->state = LV_INDEV_STATE_REL;
//     }
//     else
//     {
//         data->state = LV_INDEV_STATE_PR;

//         /*Set the coordinates*/
//         data->point.x = touchX;
//         data->point.y = touchY;

//         //Serial.print( "Data x " );
//         //Serial.println( touchX );

//         //Serial.print( "Data y " );
//         //Serial.println( touchY );
//     }
// }

// static bool keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
// {
//     static uint32_t last_key = 0;

//     /*Get the current x and y coordinates*/
//     mouse_get_xy(&data->point.x, &data->point.y);

//     /*Get whether the a key is pressed and save the pressed key*/
//     uint32_t act_key = keypad_get_key();
//     if(act_key != 0) {
//         data->state = LV_INDEV_STATE_PR;

//         /*Translate the keys to LVGL control characters according to your key definitions*/
//         switch(act_key) {
//         case 1:
//             act_key = LV_KEY_NEXT;
//             break;
//         case 2:
//             act_key = LV_KEY_PREV;
//             break;
//         case 3:
//             act_key = LV_KEY_LEFT;
//             break;
//         case 4:
//             act_key = LV_KEY_RIGHT;
//             break;
//         case 5:
//             act_key = LV_KEY_ENTER;
//             break;
//         }
//         last_key = act_key;
//     } else {
//         data->state = LV_INDEV_STATE_REL;
//     }

//     data->key = last_key;

//     /*Return `false` because we are not buffering and no more data to read*/
//     return false;
// }
// static uint32_t keypad_get_key(void)
// {
//     /*Your code comes here*/
//     //这是我添加获取按键值相关的操作
// 	if(digitalRead(inputPin) == 0){
// 		return 1;	//和 LV_KEY_NEXT 对应 
//     Serial.println("inputpin=14");
// 	}
//     return 0;
// }

// 定义键盘按键的GPIO端口
#define BUTTON_PIN_21 32//上
#define BUTTON_PIN_19 33//下
#define BUTTON_PIN_18 25//左
#define BUTTON_PIN_15 26//右
#define BUTTON_PIN_2 27 //确定

// 防抖时间
// 防抖时间
#define DEBOUNCE_TIME 100

// 存储上一次按钮的状态和上次更新时间
static uint32_t lastState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH};
static unsigned long lastDebounceTime[5] = {0, 0, 0, 0, 0};
static int buttonPins[5] = {BUTTON_PIN_21, BUTTON_PIN_19, BUTTON_PIN_18, BUTTON_PIN_15, BUTTON_PIN_2};

// 初始化键盘端口为输入
static void keypad_init(void) {
    for (int i = 0; i < 5; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }
}

// 获取键值，处理防抖
static uint32_t keypad_get_key(void) {
    unsigned long currentTime = millis();
    for (int i = 0; i < 5; i++) {
        uint32_t state = digitalRead(buttonPins[i]);
       if (state != lastState[i]) {
           if (currentTime - lastDebounceTime[i] > DEBOUNCE_TIME) {
                lastDebounceTime[i] = currentTime;
                lastState[i] = state;
                if (state == LOW) {
                    Serial.print("key=");
                    Serial.println(buttonPins[i]);
                    return i + 1;  // 返回键值，从1开始计数
                }
            }
        }
    }
    return 0; // 如果没有按键被按下，返回0
}

// 读取键盘设备状态，更新LVGL输入设备
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    static uint32_t last_key = 0;
    static bool isPressed = false;

    uint32_t act_key = keypad_get_key();
    if (act_key != 0 && !isPressed) {
        data->state = LV_INDEV_STATE_PR;
        isPressed = true;

        // 将数字键映射到LVGL控制字符
        switch (act_key) {
            case 1: data->key = LV_KEY_UP; break;
            case 2: data->key = LV_KEY_DOWN; break;
            case 3: data->key = LV_KEY_NEXT; break;
            case 4: data->key = LV_KEY_PREV; break;
            case 5: data->key = LV_KEY_ENTER; break;
            default: break;
        }
        last_key = data->key;
    } else if (act_key == 0 && isPressed) {
        data->state = LV_INDEV_STATE_REL;
        isPressed = false;
    }

    data->key = last_key; // 设置最后一个按键为当前键
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char * txt = lv_btnmatrix_get_btn_text(obj, id);
 
        LV_LOG_USER("%s was pressed\n", txt);
    }
}
uint8_t Test_Mode;      // 用于区分调试模式与用户模式
void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */
    String LVGL_Arduino = "Hello Arduino! ";
    // LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

     Serial.println( LVGL_Arduino );
    // Serial.println( "I am LVGL_Arduino" );

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

    /*Set the touchscreen calibration data,
     the actual data for your display can be acquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library*/

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    //Test_Mode = 1;
    Test_Mode = digitalRead(14);   //用于调试模式
    Serial.printf("Test_Mode = %d \r\n",Test_Mode);

    pinMode(13, OUTPUT);    //植入端引脚控制初始化
    static lv_indev_drv_t indev_drv;
    keypad_init();//进行键盘的初始化定义

    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register( &indev_drv );
    //touch_calibrate();

    ui_init();
    tft.begin(); 
    tft.invertDisplay(false);         /* TFT init */
    tft.setRotation(2); /* Landscape orientation, flipped */

    _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_NONE, 1000, 0, &ui_Screen1_screen_init);
    _ui_screen_delete(&ui_Screen1);
    Serial.println( "Setup done" );
     //delay(2000);
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay( 100 );
    //Serial.println("function search is running on core: ");
   //Serial.println(xPortGetCoreID());
}
