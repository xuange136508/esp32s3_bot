#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "protocol.h"
#include "websocket_protocol.h"
#include "wifi_manager.h"

#include <driver/gpio.h>
#include <driver/i2s_pdm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <driver/i2s_std.h>

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_common.h>

#include "display/lcd_display.h"
#include <string>
#include <mutex>
#include <list>
#include "MyApp.h"
#include "esp_psram.h"



extern "C" void app_main(void)
{
    // 初始化事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 初始化 NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    printf("开始配置网络！\n");
    const char* ssid = "mama"; 
    const char* password = "Gzscmama.cn";
    WiFiManager wifiManager(ssid, password); 
    wifiManager.init();  // 连接wifi


    // 检查 PSRAM 是否启用
    // if (esp_psram_is_initialized()) {
    //     ESP_LOGI("PSRAM", "可用: %d MB", esp_psram_get_size() / (1024 * 1024));
    // } else {
    //     ESP_LOGW("PSRAM", "PSRAM未启用，实际分配到内部RAM");
    // }

    
    // 保持运行
    //  while (true) {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }


    // 创建播放器实例
    // PCMPlayer player;
    // // 获取嵌入的音频数据
    // extern const int16_t audio_pcm_start[] asm("_binary_snow_pcm_start");
    // extern const int16_t audio_pcm_end[]   asm("_binary_snow_pcm_end");
    // size_t audio_size = audio_pcm_end - audio_pcm_start;
    // ESP_LOGI(TAG, "Playing PCM audio (size: %d bytes)...", audio_size);
    // player.playMonoPCM(audio_pcm_start, audio_size);
    // ESP_LOGI(TAG, "Playback finished");
    // // 保持运行
    // while (true) {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }



    // 连接wifi
    // printf("start mama wifi connect!!!\n");
    // const char* ssid = "mama"; 
    // const char* password = "Gzscmama.cn";
    // WiFiManager wifiManager(ssid, password);
    // wifiManager.init();




    // 倒计时关闭重启
    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);
    // esp_restart();
}



// #include <cstdio>
// #include <cstring>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/i2s.h"
// #include "esp_log.h"
// #include "esp_spiffs.h"
// #include "esp_system.h"
// #include <cmath>

// I2S 配置
// #define I2S_NUM             (I2S_NUM_0)

// #define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
// #define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
// #define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6
// #define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
// #define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
// #define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

// #define AUDIO_INPUT_SAMPLE_RATE  16000
// #define AUDIO_OUTPUT_SAMPLE_RATE 24000

// i2s_chan_handle_t tx_handle_ = nullptr;

// // 缓冲区大小
// #define BUFFER_SIZE         (1024)

// class PCMPlayer {
// public:
//     PCMPlayer() {
//         init_i2s();
//     }

//     ~PCMPlayer() {
//         i2s_driver_uninstall(I2S_NUM);
//     }

//     void playMonoPCM(const int16_t* data, int samples) {
//         std::vector<int16_t> buffer(samples);// 创建 32 位缓冲区
    
//         // output_volume_: 0-100
//         // volume_factor_: 0-65536
//         int32_t volume_factor = pow(double(50) / 100.0, 2) * 65536; // 计算音量因子
//         for (int i = 0; i < samples; i++) {
//             int32_t temp = int32_t(data[i]) * volume_factor; // 使用 int64_t 进行乘法运算
//             if (temp > INT16_MAX) { // 限制输出值在 INT32 范围内
//                 buffer[i] = INT16_MAX;
//             } else if (temp < INT16_MIN) {
//                 buffer[i] = INT16_MIN;
//             } else {
//                 buffer[i] = static_cast<int16_t>(temp);// 转换为 32 位整数
//             }
//         }
    
//         size_t bytes_written;
//         // 将缓冲区中的数据写入 I2S 通道
//         ESP_ERROR_CHECK(i2s_channel_write(tx_handle_, buffer.data(), samples * sizeof(int16_t), &bytes_written, portMAX_DELAY));
//         // 返回实际写入的样本数
//         // return bytes_written / sizeof(int32_t); 
//     }

// private:
//     void init_i2s() {
//         // Create a new channel for speaker
//     i2s_chan_config_t chan_cfg = {
//         .id = (i2s_port_t)0,
//         .role = I2S_ROLE_MASTER,
//         .dma_desc_num = 6,
//         .dma_frame_num = 240,
//         .auto_clear_after_cb = true,
//         .auto_clear_before_cb = false,
//         .intr_priority = 0,
//     };
//     ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, nullptr));

//     i2s_std_config_t std_cfg = {
//         .clk_cfg = {
//             .sample_rate_hz = (uint16_t)16000,
//             .clk_src = I2S_CLK_SRC_DEFAULT,
//             .mclk_multiple = I2S_MCLK_MULTIPLE_256,
// 			#ifdef   I2S_HW_VERSION_2
// 				.ext_clk_freq_hz = 0,
// 			#endif

//         },
//         .slot_cfg = {
//             .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
//             .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
//             .slot_mode = I2S_SLOT_MODE_MONO,
//             .slot_mask = I2S_STD_SLOT_LEFT,
//             .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
//             .ws_pol = false,
//             .bit_shift = true,
//             #ifdef   I2S_HW_VERSION_2
//                 .left_align = true,
//                 .big_endian = false,
//                 .bit_order_lsb = false
//             #endif

//         },
//         .gpio_cfg = {
//             .mclk = I2S_GPIO_UNUSED,
//             .bclk = AUDIO_I2S_SPK_GPIO_BCLK,
//             .ws = AUDIO_I2S_SPK_GPIO_LRCK,
//             .dout = AUDIO_I2S_SPK_GPIO_DOUT,
//             .din = I2S_GPIO_UNUSED,
//             .invert_flags = {
//                 .mclk_inv = false,
//                 .bclk_inv = false,
//                 .ws_inv = false
//             }
//         }
//     };
//     ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));// 初始化扬声器通道

//     }
// };
