#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>
#include <cstring>

#include "display.h"
#include "font_awesome_symbols.h"

#define TAG "Display"

Display::Display() {
    // 从设置中加载主题
    // Settings settings("display", false);
    // current_theme_name_ = settings.GetString("theme", "light"); // 获取主题名称，默认为 "light"
    current_theme_name_ = "light";
 
    // 通知定时器
    esp_timer_create_args_t notification_timer_args = {
        .callback = [](void *arg) {
            Display *display = static_cast<Display*>(arg);
            DisplayLockGuard lock(display); // 创建一个锁以保护对 Display 对象的访问
            lv_obj_add_flag(display->notification_label_, LV_OBJ_FLAG_HIDDEN); // 隐藏通知标签
            lv_obj_clear_flag(display->status_label_, LV_OBJ_FLAG_HIDDEN); // 显示状态标签
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,  // 指定定时器的调度方法
        .name = "notification_timer",   // 定时器的名称
        .skip_unhandled_events = false,  // 不跳过未处理的事件
    };
    ESP_ERROR_CHECK(esp_timer_create(&notification_timer_args, &notification_timer_));

    // 创建更新显示的定时器
    esp_timer_create_args_t update_display_timer_args = {
        .callback = [](void *arg) {
            // Display *display = static_cast<Display*>(arg);
            // display->Update();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "display_update_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&update_display_timer_args, &update_timer_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(update_timer_, 1000000)); // 每秒更新一次显示

    // Create a power management lock
    // 创建一个电源管理锁以限制 CPU 的频率
    auto ret = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "display_update", &pm_lock_);
    if (ret == ESP_ERR_NOT_SUPPORTED) { // 检查是否支持电源管理
        ESP_LOGI(TAG, "Power management not supported");
    } else {
        ESP_ERROR_CHECK(ret);
    }
}

Display::~Display() {
    if (notification_timer_ != nullptr) {
        esp_timer_stop(notification_timer_);
        esp_timer_delete(notification_timer_);
    }
    if (update_timer_ != nullptr) {
        esp_timer_stop(update_timer_);
        esp_timer_delete(update_timer_);
    }

    if (network_label_ != nullptr) {
        lv_obj_del(network_label_);
        lv_obj_del(notification_label_);
        lv_obj_del(status_label_);
        lv_obj_del(mute_label_);
        lv_obj_del(battery_label_);
        lv_obj_del(emotion_label_);
    }

    if (pm_lock_ != nullptr) {
        esp_pm_lock_delete(pm_lock_);
    }
}

void Display::SetStatus(const char* status) {
    DisplayLockGuard lock(this);
    if (status_label_ == nullptr) {
        return;
    }
    lv_label_set_text(status_label_, status);
    lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
}

void Display::ShowNotification(const std::string &notification, int duration_ms) {
    ShowNotification(notification.c_str(), duration_ms);
}

void Display::ShowNotification(const char* notification, int duration_ms) {
    DisplayLockGuard lock(this); // 创建一个锁，用于保护共享资源，确保线程安全
    if (notification_label_ == nullptr) {
        return;
    }
    lv_label_set_text(notification_label_, notification);
    lv_obj_clear_flag(notification_label_, LV_OBJ_FLAG_HIDDEN); // 清除 notification_label_ 的隐藏标志，使其可见
    lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN); // 将 status_label_ 设置为隐藏

    esp_timer_stop(notification_timer_);
    ESP_ERROR_CHECK(esp_timer_start_once(notification_timer_, duration_ms * 1000));
}




void Display::SetIcon(const char* icon) {
    DisplayLockGuard lock(this);
    if (emotion_label_ == nullptr) {
        return;
    }
    lv_label_set_text(emotion_label_, icon);
}

void Display::SetChatMessage(const char* role, const char* content) {
    DisplayLockGuard lock(this);
    if (chat_message_label_ == nullptr) {
        return;
    }
    lv_label_set_text(chat_message_label_, content);
}

