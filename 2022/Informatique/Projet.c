#include "stm32l1xx.h"
#include <stdlib.h>
#include <math.h>

void TIM2_IRQ_Config();
void DAC1_Config();
void DAC1_Set_Quickly(uint16_t value);
void ADC1_Init(uint8_t ch);
uint16_t Get_Adc_Quickly();




/**
 *  Programme principal (hors interruptions)
 */
int main(void)
{
    DAC1_Config();
    ADC1_Init(ADC_Channel_5);

    // # LED sur PB7
    /* Activer GPIOB sur AHB */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB,ENABLE);
    /* Configurer PB7 comme sortie tout-ou-rien */
    GPIO_InitTypeDef gpio_b;
    GPIO_StructInit(&gpio_b);
    gpio_b.GPIO_Mode = GPIO_Mode_OUT;
    gpio_b.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOB,&gpio_b);



    TIM2_IRQ_Config(); // lancer l'interruption maintenant que tout le reste est pret

    while(1) {


    }
}









/**
 *  Gestion du TIM2 par interruption pour une frequence d'echantillonnage
 */
// callback pour l'interruption periodique associee a TIM2
void TIM2_IRQHandler() {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        //GPIO_ToggleBits(GPIOB,GPIO_Pin_7);
        GPIOB->ODR ^= GPIO_Pin_7;
        uint16_t adc = Get_Adc_Quickly();
        DAC1_Set_Quickly(adc);
        GPIOB->ODR ^= GPIO_Pin_7;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

// ### TIMER 2 + IRQ a 44000 Hz
// Configuration Timer 2 a 44000 Hz
// avec emission d'IRQ : execute periodiquement TIM2_IRQHandler()
void TIM2_IRQ_Config()
{
    /*Activer TIM2 sur APB1 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
    /* Configurer TIM2 a 500 ms */
    TIM_TimeBaseInitTypeDef timer_2;
    TIM_TimeBaseStructInit(&timer_2);
    timer_2.TIM_Prescaler = 0;
    timer_2.TIM_Period = 363;// periode = (1/16e6) * (120+1)*(2+1) pour 440 Hz par exemple
    TIM_TimeBaseInit(TIM2,&timer_2);
    TIM_SetCounter(TIM2,0);
    TIM_Cmd(TIM2, ENABLE);

    /* Associer une interruption a TIM2 */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic;
    /* Configuration de l'interruption */
    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 1;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}






/**
 *  Gestion du DAC1 (= DAC Channel 1) sur PA4
 */
// Configuration
void DAC1_Config()
{
    /*Activer GPIOA sur AHB */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    /* Configurer PA4 en mode analogique*/
    GPIO_InitTypeDef gpio_a;
    GPIO_StructInit(&gpio_a);
    gpio_a.GPIO_Mode  = GPIO_Mode_AN;
    gpio_a.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA, &gpio_a);

    /*Activer DAC sur APB1 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    /* Configurer DAC1 avec parametres par defaut */
    DAC_InitTypeDef dac_1;
    DAC_StructInit(&dac_1);
    DAC_Init(DAC_Channel_1, &dac_1);
    /* Activer DAC1 */
    DAC_Cmd(DAC_Channel_1, ENABLE);
}




void DAC1_Set_Quickly(uint16_t value)
{
    static __IO uint32_t tmp = (uint32_t)DAC_BASE + (uint32_t)0x00000008 + DAC_Align_12b_R;
    *(__IO uint32_t *) tmp = value;
}






/**
 *  Gestion du ADC sur PA5 en mode "one shot"
 */

// # PA5 comme entree analogique
void ADC1_Init(uint8_t ch)
{
    // #\protect \unhbox \voidb@x \protect \penalty \@M \ entree analogique : PA5
    // ## Configuration du module GPIO pour PA5
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    GPIO_InitTypeDef gpio_PA5;
    gpio_PA5.GPIO_Pin = GPIO_Pin_5;
    gpio_PA5.GPIO_Mode = GPIO_Mode_AN;
    GPIO_Init(GPIOA, &gpio_PA5);

    // ## Configuration du DAC pour PA5
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    ADC_InitTypeDef adc_1;
    ADC_StructInit(&adc_1);

    adc_1.ADC_ContinuousConvMode = DISABLE;
    adc_1.ADC_NbrOfConversion = 1;
    adc_1.ADC_Resolution = ADC_Resolution_12b;
    adc_1.ADC_ExternalTrigConv = 0;//ADC_ExternalTrigConv_T2_CC3;
    adc_1.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_Init(ADC1, &adc_1);
    ADC_Cmd(ADC1, ENABLE);

    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_4Cycles);

}


// # fonction plus optimisee basee sur les registres (2 x plus rapide)
uint16_t Get_Adc_Quickly() {
    ADC1->CR2 |= (uint32_t)ADC_CR2_SWSTART;
    while((ADC1->SR & ADC_FLAG_EOC) == 0) ;
    return (uint16_t) ADC1->DR;
}

