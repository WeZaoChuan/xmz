#include "btn_app.h"
#include "ebtn.h"
#include "gpio.h"
#include "adc_app.h"

extern uint8_t ucLed[6];
extern WaveformInfo wave_data;
// Ƶʵڲֵ
#define FREQ_STEP 100 // ÿ/100Hz

// ηȵڲֵ
#define AMP_STEP 100 // ÿ/100mV

// ǰͺСƵ
static dac_waveform_t current_wave_type = WAVEFORM_SINE;
#define MIN_FREQUENCY 100   // СƵ100Hz
#define MAX_FREQUENCY 50000 // Ƶ10kHz

// η
#define MIN_AMPLITUDE 100  // Сֵ100mV
#define MAX_AMPLITUDE 1650 // ֵ1.65V (3.3Vһ)

/*
    ֵMAX_AMPLITUDE = 1650mV
    DACοѹ(DAC_VREF_MV)Ϊ3300mV (3.3V)
    Χĵѹ(VREF/2)±仯
    ֵȾĵѹֵ3300mV/2 = 1650mV
    ȷβᳬDACΧ0-3.3V
    εķֵΪ3.3V


    ֵMIN_AMPLITUDE = 100mV
    Ϊ100mVΪ֤һĿɼ
    ķʵӦԹ۲ʹ
    ĻʵԿǵֵ
*/

uint32_t new_freq;
uint32_t current_freq;
uint16_t current_amp;
uint16_t new_amp;
uint8_t uart_send_flag = 0;

typedef enum
{
    USER_BUTTON_0 = 0,
    USER_BUTTON_1,
    USER_BUTTON_2,
    USER_BUTTON_3,
    USER_BUTTON_4,
    USER_BUTTON_5,
    USER_BUTTON_MAX,

    //    USER_BUTTON_COMBO_0 = 0x100,
    //    USER_BUTTON_COMBO_1,
    //    USER_BUTTON_COMBO_2,
    //    USER_BUTTON_COMBO_3,
    //    USER_BUTTON_COMBO_MAX,
} user_button_t;

/*  Debounce time in milliseconds, Debounce time in milliseconds for release event, Minimum pressed time for valid click event, Maximum ...,
    Maximum time between 2 clicks to be considered consecutive click, Time in ms for periodic keep alive event, Max number of consecutive clicks */
static const ebtn_btn_param_t defaul_ebtn_param = EBTN_PARAMS_INIT(20, 0, 20, 1000, 0, 1000, 10);

static ebtn_btn_t btns[] = {
    EBTN_BUTTON_INIT(USER_BUTTON_0, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_1, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_2, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_3, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_4, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_5, &defaul_ebtn_param),
};

// static ebtn_btn_combo_t btns_combo[] = {
//     EBTN_BUTTON_COMBO_INIT_RAW(USER_BUTTON_COMBO_0, &defaul_ebtn_param, EBTN_EVT_MASK_ONCLICK),
//     EBTN_BUTTON_COMBO_INIT_RAW(USER_BUTTON_COMBO_1, &defaul_ebtn_param, EBTN_EVT_MASK_ONCLICK),
// };

uint8_t prv_btn_get_state(struct ebtn_btn *btn)
{
    switch (btn->key_id)
    {
    case USER_BUTTON_0:
        return !HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15);
    case USER_BUTTON_1:
        return !HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_13);
    case USER_BUTTON_2:
        return !HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_11);
    case USER_BUTTON_3:
        return !HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9);
    case USER_BUTTON_4:
        return !HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7);
    case USER_BUTTON_5:
        return !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    default:
        return 0;
    }
}

// void prv_btn_event(struct ebtn_btn *btn, ebtn_evt_t evt)
//{
//     if ((btn->key_id == USER_BUTTON_0) && (ebtn_click_get_count(btn) == 1))
//     {
//         ucLed[0] ^= 1;
//         uart_send_flag ^= 1;
//         wave_analysis_flag = 1;
//     }

//    if ((btn->key_id == USER_BUTTON_1) && (ebtn_click_get_count(btn) == 1))
//    {
//        // 1л
//        ucLed[1] ^= 1;

//        // лͣҲ->->ǲ->Ҳ...
//        switch (current_wave_type)
//        {
//        case WAVEFORM_SINE:
//            current_wave_type = WAVEFORM_SQUARE;
//            break;
//        case WAVEFORM_SQUARE:
//            current_wave_type = WAVEFORM_TRIANGLE;
//            break;
//        case WAVEFORM_TRIANGLE:
//            current_wave_type = WAVEFORM_SINE;
//            break;
//        default:
//            current_wave_type = WAVEFORM_SINE;
//            break;
//        }

//        // µĲ
//        dac_app_set_waveform(current_wave_type);
//    }

//    if ((btn->key_id == USER_BUTTON_2) && (ebtn_click_get_count(btn) == 1))
//    {
//        // 2Ƶ
//        ucLed[2] ^= 1;

//        // ȡǰƵȻFREQ_STEP
//        current_freq = dac_app_get_update_frequency() / WAVEFORM_SAMPLES;
//        new_freq = current_freq + FREQ_STEP;

//        // Ƶ
//        if (new_freq > MAX_FREQUENCY)
//            new_freq = MAX_FREQUENCY;

//        // Ƶ
//        dac_app_set_frequency(new_freq);
//    }

//    if ((btn->key_id == USER_BUTTON_3) && (ebtn_click_get_count(btn) == 1))
//    {
//        // 3Ƶ
//        ucLed[3] ^= 1;

//        // ȡǰƵȻFREQ_STEP
//        current_freq = dac_app_get_update_frequency() / WAVEFORM_SAMPLES;
//        new_freq = (current_freq > FREQ_STEP) ? (current_freq - FREQ_STEP) : MIN_FREQUENCY;

//        // СƵ
//        if (new_freq < MIN_FREQUENCY)
//            new_freq = MIN_FREQUENCY;

//        // Ƶ
//        dac_app_set_frequency(new_freq);
//    }

//    if ((btn->key_id == USER_BUTTON_4) && (ebtn_click_get_count(btn) == 1))
//    {
//        // 4ӷֵ
//        ucLed[4] ^= 1;

//        // ȡǰֵȲAMP_STEP
//        current_amp = dac_app_get_amplitude();
//        new_amp = current_amp + AMP_STEP;

//        // 
//        if (new_amp > MAX_AMPLITUDE)
//            new_amp = MAX_AMPLITUDE;

//        // ·
//        dac_app_set_amplitude(new_amp);
//    }

//    if ((btn->key_id == USER_BUTTON_5) && (ebtn_click_get_count(btn) == 1))
//    {
//        // 5ٷֵ
//        ucLed[5] ^= 1;

//        // ȡǰֵȲAMP_STEP
//        current_amp = dac_app_get_amplitude();
//        new_amp = (current_amp > AMP_STEP) ? (current_amp - AMP_STEP) : MIN_AMPLITUDE;

//        // С
//        if (new_amp < MIN_AMPLITUDE)
//            new_amp = MIN_AMPLITUDE;

//        // ·
//        dac_app_set_amplitude(new_amp);
//    }
//}


// Ϊȫֱļʹ
volatile uint8_t oled_display_state = 0; // 0:WuZhaowei:19, 1:Zhoujing, 2:3638, 3:1 5 1
volatile uint8_t oled_update_flag = 1;   // ʼҪ

// ¼
void prv_btn_event(struct ebtn_btn *btn, ebtn_evt_t evt)
{
    if(evt == EBTN_EVT_ONCLICK)
    {
        uint16_t click_cnt = ebtn_click_get_count(btn);

        switch(btn->key_id)
        {
            case USER_BUTTON_0:  // 按键1
                if(click_cnt == 1)
                {
                    oled_display_state = 0;  // 保持原有逻辑
                    ucLed[0]=1; ucLed[5]=0; ucLed[4]=1;
                    ucLed[2]=1; ucLed[1]=0; ucLed[3]=0;
                    oled_update_flag = 1;    // 保持原有逻辑
                }
                break;
                
            case USER_BUTTON_1:  // 按键2
                if(click_cnt == 1)
                {
                    oled_display_state = 1;  //  
                    // LED控制保持原有逻辑
                    ucLed[0]=1; ucLed[5]=1; ucLed[4]=1;
                    ucLed[2]=1; ucLed[1]=1; ucLed[3]=1;
                    oled_update_flag = 1;    // 保持原有逻辑
                }
                break;

            case USER_BUTTON_2:  // 按键3
                if(click_cnt == 1)
                {
                    oled_display_state = 2;  // 显示3638
                    oled_update_flag = 1;    // 保持原有逻辑
                }
                break;

            case USER_BUTTON_3:  // 按键4
                if(click_cnt == 1)
                {
                    oled_display_state = 3;  // 显示1 5 1
                    oled_update_flag = 1;    // 保持原有逻辑
                }
                break;

            case USER_BUTTON_4:  // 按键5 (GPIO PE7) - 开始ADC数据记录
                if(click_cnt == 1)
                {
                    ucLed[4] ^= 1;  // LED4状态
                    adc_start_recording();  // 开始记录ADC数据

                    // 记录一次时间，自动停止录音（假设为开始记录）
                    // 实际记录数据由adc_task处理
                    // 确保记录结束时自动停止
                }
                break;

            case USER_BUTTON_5:  // 按键6 (GPIO PB0) - 停止录音并保存到SD读取打印
                if(click_cnt == 1)
                {
                    ucLed[5] ^= 1;  // LED5状态

                    // 停止录音并停止记录
                    if (adc_record_flag)
                    {
                        adc_stop_recording();
                        adc_save_data_to_sd();
                    }

                    // 从SD读取并打印数据
                    adc_print_data_from_sd();
                }
                break;
        }
    }
}




void app_btn_init(void)
{
    // ebtn_init(btns, EBTN_ARRAY_SIZE(btns), btns_combo, EBTN_ARRAY_SIZE(btns_combo), prv_btn_get_state, prv_btn_event);
    ebtn_init(btns, EBTN_ARRAY_SIZE(btns), NULL, 0, prv_btn_get_state, prv_btn_event);

    //    ebtn_combo_btn_add_btn(&btns_combo[0], USER_BUTTON_0);
    //    ebtn_combo_btn_add_btn(&btns_combo[0], USER_BUTTON_1);

    //    ebtn_combo_btn_add_btn(&btns_combo[1], USER_BUTTON_2);
    //    ebtn_combo_btn_add_btn(&btns_combo[1], USER_BUTTON_3);

    HAL_TIM_Base_Start_IT(&htim14);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == (&htim14))
    {
        ebtn_process(uwTick);
    }
}

void btn_task(void)
{
	
}






