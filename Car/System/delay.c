#include "delay.h"

volatile unsigned int delay_times = 0;

void delay_us(unsigned long __us) 
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 38;

    // ������Ҫ��ʱ���� = �ӳ�΢���� * ÿ΢����ʱ����
	// Calculate the number of clocks required = delay microseconds * number of clocks per microsecond
    ticks = __us * (32000000 / 1000000);

    // ��ȡ��ǰ��SysTickֵ
	// Get the current SysTick value
    told = SysTick->VAL;

    while (1)
    {
        // �ظ�ˢ�»�ȡ��ǰ��SysTickֵ
		// Repeatedly refresh to get the current SysTick value
        tnow = SysTick->VAL;

        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += SysTick->LOAD - tnow + told;

            told = tnow;

            // �����ﵽ����Ҫ��ʱ���������˳�ѭ��
			// If the required number of clocks is reached, exit the loop
            if (tcnt >= ticks)
                break;
        }
    }
}

void delay_ms(uint32_t ms)
{
    delay_us(ms * 1000);
}