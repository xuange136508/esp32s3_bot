#include "wifi_board.h"
#include "audio_codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "button.h"
#include "myApp.h"
#include "settings.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_common.h>

#include <driver/gpio.h>

#define TAG "CompactWifiBoardLCD"

LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_awesome_16_4);



#define AUDIO_INPUT_SAMPLE_RATE  16000
// #define AUDIO_OUTPUT_SAMPLE_RATE 16000 小智AI配置
#define AUDIO_OUTPUT_SAMPLE_RATE 8000  

/*
================ Arduino打印驱动参数 ==============================
10:25:20.267 -> Setup ID: 0
10:25:20.267 -> ESP Processor Code: 50
10:25:20.267 -> SPI Transaction Support: 1
10:25:20.267 -> Serial or Parallel: 1
10:25:20.267 -> SPI Port: 255
10:25:20.267 -> ESP8266 Overlap Mode: 0
10:25:20.267 -> TFT Driver: 7789
10:25:20.267 -> TFT Width: 240
10:25:20.267 -> TFT Height: 320
10:25:20.267 -> Pin MOSI: 47
10:25:20.267 -> Pin MISO: 48
10:25:20.267 -> Pin CLK: 21
10:25:20.267 -> Pin CS: 42
10:25:20.311 -> Pin DC: 41
10:25:20.311 -> Pin RD: -1
10:25:20.311 -> Pin WR: -1
10:25:20.311 -> Pin RST: -1
10:25:20.311 -> Parallel Pins (D0-D7): -1, -1, -1, -1, -1, -1, -1, -1
10:25:20.311 -> Pin LED: 44
10:25:20.311 -> Pin LED On: 2
10:25:20.311 -> Pin Touch CS: 46
10:25:20.311 -> TFT SPI Frequency: 270
10:25:20.311 -> TFT Read Frequency: 200
10:25:20.311 -> Touch SPI Frequency: 25
==============================================
接线说明：TFTLCD液晶模块-->ESP32 IO
        (GND)-->(GND)
        (VCC)-->(3V3)
        (SCL)-->(21)
        (SDI)-->(47)
        (RST)-->(EN)
        (RS)-->(41)
        (BL)-->(14)
        (SDO)-->(48)
        (CS)-->(42)
        (TP_CS)-->(46)
        (TP_PEN)-->(2)
触摸显示屏（st7789驱动）硬件引脚：
    CLK MOSI RST DC BLK MISO CS1 CS2 PEN
*/

// 显示屏参数
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_SPI_MODE 0

// 显示屏引脚
#define DISPLAY_DC_PIN          GPIO_NUM_41
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_42
#define DISPLAY_SPI_CLK_PIN      GPIO_NUM_21
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_47
#define DISPLAY_SPI_MISO_PIN    GPIO_NUM_48

// i2s 引脚（麦克风、喇叭外接）
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

// 按钮
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40 // key 1 音量+
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39 // key 2 音量-
#define AUDIO_BUTTON_GPIO       GPIO_NUM_1 // key 3 音频录入
#define AGENT_BUTTON_GPIO       GPIO_NUM_38 // key 4 切换智能体

// 字体声明
LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_awesome_16_4);


class CompactWifiBoardLCD : public WifiBoard {
private:
    Button volume_up_button_;
    Button volume_down_button_;
    Button audio_button_;
    Button agent_button_;
    LcdDisplay* display_;

    // SPI初始化（用于显示屏）
    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = DISPLAY_SPI_MISO_PIN;
        buscfg.sclk_io_num = DISPLAY_SPI_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // 显示屏初始化（以ST7789为例）
    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;
        
        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 80 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB; //用于指定 RGB 元素的顺序
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
        
        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, false); // 控制颜色反转的参数，黑白颠倒，如果是IPS屏可以为true
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        
        // 创建显示屏对象
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                                    DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
                                    DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY,
                                    {
                                        .text_font = &font_puhui_16_4,
                                        .icon_font = &font_awesome_16_4,
                                        .emoji_font = font_emoji_32_init(),
                                    });

        // 初始化屏幕对象
        display_->initScreen();

        // 绘制小金猴动画                            
        // display_->drawforkStory();
    }

 
    void InitializeButtons() {
        agent_button_.OnClick([this]() {
            try {
                // 找到匹配的智能体ID
                std::vector<std::shared_ptr<AiRole>> m_roles = MyApp::GetInstance().getAgentList();
                Settings agentSettings("agentId", true);
                int agentIndex = agentSettings.GetInt("agentIndex", 0);  
                if(agentIndex < 5){
                    agentIndex++;
                }else{
                    agentIndex = 0;
                }
                agentSettings.SetInt("agentIndex", agentIndex);  // 设置智能体游标
                std::shared_ptr<AiRole> role = m_roles.at(agentIndex);
                agentSettings.SetString("agentId", role.get()->getId());  // 设置智能体ID
                ESP_LOGI(APP_TAG, "按键切换智能体：%s 游标为: %s",role.get()->getId().c_str(), std::to_string(agentIndex).c_str());

                // 渲染智能体对应的动画
                drawAgentImage(role.get()->getImageUrl());

            } catch (const std::exception& e) {
                ESP_LOGE(TAG, "board Exception: %s", e.what());
            } catch (...) {
                ESP_LOGE(TAG, "Unknown exception");
            }
        });
        audio_button_.OnPressDown([this]() {
            try {
                Settings agentSettings("agentId", true);
                int agentIndex = agentSettings.GetInt("agentIndex", 0);  
                std::vector<std::shared_ptr<AiRole>> m_roles = MyApp::GetInstance().getAgentList();
                std::shared_ptr<AiRole> role = m_roles.at(agentIndex);
                drawAgentImage(role.get()->getHearImageUrl());
            } catch (...) {
                ESP_LOGE(TAG, "Unknown exception");
            }
            ESP_LOGI(TAG, "开始录音"); 
            MyApp::GetInstance().StartListening();
        });
        audio_button_.OnPressUp([this]() {
            ESP_LOGI(TAG, "结束录音"); 
            MyApp::GetInstance().StopListening();
        });
        volume_up_button_.OnClick([this]() {
            Settings boardSettings("board", true);
            int cur = boardSettings.GetInt("volume", 30); 
            cur += 10;
            if (cur >= 100) {
                cur = 100; 
            }
            boardSettings.SetInt("volume", cur);
            ESP_LOGI(TAG, "当前音量为：%d", cur); 
        });
        volume_down_button_.OnClick([this]() {
            Settings boardSettings("board", true);
            int cur = boardSettings.GetInt("volume", 30); 
            cur -= 10; 
            if (cur <= 10) {
                cur = 10; // 限制最小音量为10
            }
            boardSettings.SetInt("volume", cur); 
            ESP_LOGI(TAG, "当前音量为：%d", cur); 
        });
    }


public:
    CompactWifiBoardLCD() :
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO),
        audio_button_(AUDIO_BUTTON_GPIO),
        agent_button_(AGENT_BUTTON_GPIO)  {
        
        // 初始化SPI
        InitializeSpi();
        // 初始化LCD显示屏
        InitializeDisplay();
        // 初始化按钮
        InitializeButtons();
    }


    virtual AudioCodec* GetAudioCodec() override {

        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);

        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }


    virtual void TurnOffDisplay() override {
        esp_lcd_panel_disp_off(display_->getPanelObj(), true);
    }

    virtual void TurnOnDisplay() override {
        esp_lcd_panel_disp_off(display_->getPanelObj(), false);
    }

    
    // 绘制智能体画面
    virtual void drawAgentImage(std::string imagePath) override{
        display_->drawAgentImage(imagePath);
    }

};

DECLARE_BOARD(CompactWifiBoardLCD);
