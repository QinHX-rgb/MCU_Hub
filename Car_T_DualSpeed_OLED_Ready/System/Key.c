#include "Key.h"
/*按键扫描进程*/
uint8_t Key_Read()
{
    unsigned char temp=0;//局部变量初始化为0
    if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key1_PORT, GPIO_BUTTON_PIN_Key1_PIN)){temp = 1;}
    return temp;
}