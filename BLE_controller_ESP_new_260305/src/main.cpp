
/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include "ui_helpers.h"
#include <BLEDevice.h>
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
bool dd_edit_mode = false;      //下拉框编辑模式
bool edit_mode = false;         // 模式编辑模式
bool width_edit_mode = false;   // 脉宽编辑模式
bool time_edit_mode = false;    // 时间编辑模式
//lv_group_t * msgbox_group;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[ screenHeight * 10 ];
static lv_color_t buf2[ screenHeight * 10 ];

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */

// 定义键盘按键的GPIO端口
#define BUTTON_PIN_32 32//左
#define BUTTON_PIN_33 33//右
#define BUTTON_PIN_26 26 //确定

#define DEBOUNCE_TIME 100   // 防抖时间

// 存储上一次按钮的状态和上次更新时间
static uint32_t lastState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH};
static unsigned long lastDebounceTime[5] = {0, 0, 0, 0, 0};
static int buttonPins[3] = {BUTTON_PIN_32, BUTTON_PIN_33, BUTTON_PIN_26};

uint8_t Test_Mode;      // 用于区分调试模式与用户模式
//SemaphoreHandle_t connectSemaphore = NULL;
TimerHandle_t xTimer_15s_adc;
TaskHandle_t batteryTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = NULL;
uint32_t adc_value = 0; // 定义一个16位无符号整数变量用于存储ADC采样值
float adc_voltage = 0.0; // 定义一个浮点数变量用于存储ADC电压值
float Battery_Level = 0;
float Battery_Previous = 0; // 用于存储上一次读取的电压值
volatile uint32_t lastBackIrqTime = 0;
bool backKeyFlag = false;

// 初始化键盘端口为输入
static void keypad_init(void) {
    for (int i = 0; i < 3; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }
}

// 获取键值，处理防抖
static uint32_t keypad_get_key(void) {
    unsigned long currentTime = millis();
    for (int i = 0; i < 3; i++) {
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

        if(dd_edit_mode == true) {
            tone(15, 4000, 100);
            // 正常模式 (NEXT / PREV 切换控件)
            switch(act_key) {
                case 1: data->key = LV_KEY_UP; break;
                case 2: data->key = LV_KEY_DOWN; break;
                case 3: data->key = LV_KEY_ENTER; break;
            }
        }
        else if(edit_mode == true)
        {
            tone(15, 4000, 100);
            switch(act_key) {
                case 1: MODELEFT(); data->key = 0; break;
                case 2: MODERIGHT(); data->key = 0; break;
                case 3: data->key = LV_KEY_ENTER; break;
            }
        } 
        else if(width_edit_mode == true)
        {
            tone(15, 4000, 100);
            switch(act_key) {
                case 1: WIDTHLEFT(); data->key = 0; break;
                case 2: WIDTHRIGHT(); data->key = 0; break;
                case 3: data->key = LV_KEY_ENTER; break;
            }
        } 
        else if(time_edit_mode == true)
        {
            tone(15, 4000, 100);
            switch(act_key) {
                case 1: TIMELEFT(); data->key = 0; break;
                case 2: TIMERIGHT(); data->key = 0; break;
                case 3: data->key = LV_KEY_ENTER; break;
            }
        } 
        else {
            tone(15, 4000, 100);
            // 下拉框选择模式 (UP / DOWN 选择选项)
            switch(act_key) {
                case 1: data->key = LV_KEY_NEXT; break;
                case 2: data->key = LV_KEY_PREV; break;
                case 3: data->key = LV_KEY_ENTER; break;
            }
        }
        last_key = data->key;
    } else if (act_key == 0 && isPressed) {
        data->state = LV_INDEV_STATE_REL;
        isPressed = false;
    }
    if(data->key == 0)
    {
        data->key = 0; // 恢复上一个按键
    }
    else{
        data->key = last_key; // 设置最后一个按键为当前键
        
    }

}
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
//返回按键回调
void IRAM_ATTR btn_back_isr()
{
    uint32_t now = millis();
    if (now - lastBackIrqTime > DEBOUNCE_TIME)
    {
        if (digitalRead(25) == LOW)
        {
            tone(15, 4000, 100);
            RETURN();
            lastBackIrqTime = now;
        }
    }
}

//  电池采样
//void vTimer_15s_adc(TimerHandle_t xTimer_15s_adc)
void vTimer_15s_adc(void *pvParameters)
{
    bool is_first = false;
    bool is_Second = false;
    while(1) {
        adc_value = 0;
        adc_value = analogReadMilliVolts(4); // 从模拟输入引脚读取采样值并存储到adc_value变量中
        adc_voltage = adc_value * 0.001846f + 0.364f;
        float voltage_level = (adc_voltage - 3.0f) / 1.2f * 100.0f;

        if(is_first == true && is_Second == false)
        {
            is_Second = true;
            is_first = true; 
        }
        else if(is_Second == false && is_first == false)
        {
            is_first = true;
        }
        if(is_Second == true && is_first == true)
        {
            Battery_Previous = voltage_level;
            is_Second = true;
            is_first = false;
        }

        if(fabs(Battery_Previous - voltage_level) >= 2 && is_first == false && is_Second == true)
        {
            voltage_level = Battery_Previous;
        }
        else if(fabs(Battery_Previous - voltage_level) < 2 && is_first == false && is_Second == true)
        {
            Battery_Previous = voltage_level;
        }

        if(voltage_level >= 80)
        {
            lv_img_set_src(ui_Image3, &ui_img_controller_battery100_png);
            lv_img_set_src(ui_Image20, &ui_img_controller_battery100_png);
            lv_img_set_src(ui_Image21, &ui_img_controller_battery100_png);
        }
        if(voltage_level >= 50 && voltage_level < 80)
        {
            lv_img_set_src(ui_Image3, &ui_img_controller_battery75_png);
            lv_img_set_src(ui_Image20, &ui_img_controller_battery75_png);
            lv_img_set_src(ui_Image21, &ui_img_controller_battery75_png);
        }
        if(voltage_level >= 30 && voltage_level < 50)
        {
            lv_img_set_src(ui_Image3, &ui_img_controller_battery50_png);
            lv_img_set_src(ui_Image20, &ui_img_controller_battery50_png);
            lv_img_set_src(ui_Image21, &ui_img_controller_battery50_png);
        }
        if(voltage_level >= 10 && voltage_level < 30)
        {
            lv_img_set_src(ui_Image3, &ui_img_controller_battery25_png);
            lv_img_set_src(ui_Image20, &ui_img_controller_battery25_png);
            lv_img_set_src(ui_Image21, &ui_img_controller_battery25_png);
        }
        if(voltage_level < 10)
        {
            lv_img_set_src(ui_Image3, &ui_img_controller_battery0_png);
            lv_img_set_src(ui_Image20, &ui_img_controller_battery0_png);
            lv_img_set_src(ui_Image21, &ui_img_controller_battery0_png);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
void lvgl_task(void * pvParameters)
{
    while (1) {
        lv_timer_handler();           // 原 loop() 里的第一行
        vTaskDelay(pdMS_TO_TICKS(5)); // 等价于 usleep(5000)
    }
}
void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */
    BLEDevice::init("lzt_BLE_test");
    //String LVGL_Arduino = "Hello Arduino! ";

    lv_init();
    pinMode(15, OUTPUT);    //蜂鸣器引脚
    pinMode(25, INPUT);
    adcAttachPin(25);        //ADC校准引脚
    analogReadResolution(12);
    analogSetVRefPin(25);
    delay(100);

    pinMode(25, INPUT_PULLUP);  //返回按键
    attachInterrupt(25, btn_back_isr, FALLING);

    pinMode(22, OUTPUT);    //TFT使能
    pinMode(2, OUTPUT);
    digitalWrite(22, HIGH);
    digitalWrite(2, LOW);    
    pinMode(27, OUTPUT);//显示屏背光使能
    digitalWrite(27, HIGH);
    adcAttachPin(4);

    //connectSemaphore = xSemaphoreCreateBinary();
    /*Set the touchscreen calibration data,
     the actual data for your display can be acquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library*/

    lv_disp_draw_buf_init( &draw_buf, buf1, buf2, screenHeight * 10 );

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
    Test_Mode = digitalRead(14);   //用于调试模式
    Serial.printf("Test_Mode = %d \r\n",Test_Mode);

    pinMode(13, OUTPUT);    //植入端引脚控制初始化
    static lv_indev_drv_t indev_drv;
    keypad_init();//进行键盘的初始化定义

    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register( &indev_drv );

    ui_init();
    tft.begin(); 
    tft.invertDisplay(false);         /* TFT init */
    tft.setRotation(2); /* Landscape orientation, flipped */

    if(digitalRead(16) == 1 && digitalRead(17) == 1)
    {
        digitalWrite(2, HIGH);
        _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_NONE, 1500, 1000, &ui_Screen2_screen_init);
        _ui_screen_delete(&ui_Screen1);
    }
    else
    {
        _ui_screen_change(&ui_Screen5, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen5_screen_init);
        _ui_screen_delete(&ui_Screen1);
        start_recharge_animation();
    }
    //xTimer_15s_adc = xTimerCreate("Timer_15s_adc", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, vTimer_15s_adc);
    xTaskCreatePinnedToCore(vTimer_15s_adc, "Timer_15s_adc", 2048, NULL, 1, &batteryTaskHandle, 0);

    xTaskCreatePinnedToCore( lvgl_task, "LVGL Task", 4096, NULL, 1, &lvglTaskHandle, 1);
//    _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_NONE, 1500, 1000, &ui_Screen1_screen_init);
//    _ui_screen_delete(&ui_Screen1);
    Serial.println( "Setup done" );
}

void loop()
{
    //lv_timer_handler(); /* let the GUI do its work */
    //usleep(5000);
    vTaskDelay(100);
    //delay( 100 );
    //Serial.println("function search is running on core: ");
    //Serial.println(xPortGetCoreID());
}
