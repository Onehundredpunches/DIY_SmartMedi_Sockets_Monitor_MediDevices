#include "operating_time_manager.h" // Import các khai báo struct và hàm quản lý thời gian hoạt động
#include <EEPROM.h>                 // Thư viện EEPROM để lưu dữ liệu lâu dài
#include <Arduino.h>                // Thư viện Arduino cơ bản, cung cấp hàm millis() và các hàm tiện ích

// Hàm khởi tạo bộ đếm thời gian hoạt động
void op_time_counter_init(OperatingTimeCounter* counter, uint16_t eeprom_addr) {
    counter->eeprom_addr = eeprom_addr; // Gán địa chỉ EEPROM cho bộ đếm
    // EEPROM.begin() chỉ gọi một lần ở setup, KHÔNG gọi ở đây để tránh lỗi ghi/đọc
    EEPROM.get(eeprom_addr, counter->total_ms); // Đọc tổng thời gian hoạt động từ EEPROM

    // Kiểm tra giá trị đọc được từ EEPROM, nếu là giá trị rác hoặc quá lớn thì reset về 0
    if (counter->total_ms == 0xFFFFFFFFFFFFFFFF || counter->total_ms > (uint64_t)10*365*24*3600*1000) {
        counter->total_ms = 0; // Reset tổng thời gian hoạt động về 0
        EEPROM.put(eeprom_addr, (uint64_t)0); // Ghi giá trị 0 vào EEPROM
        EEPROM.commit(); // Lưu thay đổi vào EEPROM
    }

    // Khởi tạo các giá trị ban đầu cho các trường khác trong struct
    counter->last_on_ms = 0; // Thời điểm bắt đầu hoạt động (chưa hoạt động)
    counter->is_operating = false; // Trạng thái hiện tại (chưa hoạt động)
    counter->was_operating = false; // Trạng thái trước đó (chưa hoạt động)
    counter->last_save_ms = millis(); // Thời điểm cuối cùng lưu dữ liệu (khởi tạo bằng thời gian hiện tại)
}

// Hàm lưu tổng thời gian hoạt động vào EEPROM
static void op_time_counter_save(OperatingTimeCounter* counter) {
    EEPROM.put(counter->eeprom_addr, counter->total_ms); // Ghi tổng thời gian hoạt động vào EEPROM
    EEPROM.commit(); // Lưu thay đổi vào EEPROM
    counter->last_save_ms = millis(); // Cập nhật thời điểm cuối cùng lưu dữ liệu
}

// Hàm cập nhật trạng thái hoạt động của thiết bị
void op_time_counter_update(OperatingTimeCounter* counter, bool is_operating) {
    counter->is_operating = is_operating; // Cập nhật trạng thái hiện tại
    uint32_t now = millis(); // Lấy thời gian hiện tại (ms từ khi ESP khởi động)

    // Khi chuyển từ OFF sang ON
    if (counter->is_operating && !counter->was_operating) {
        counter->last_on_ms = now; // Ghi nhận thời điểm bắt đầu hoạt động
    }

    // Khi chuyển từ ON sang OFF
    if (!counter->is_operating && counter->was_operating) {
        if (counter->last_on_ms > 0) { // Nếu có thời điểm bắt đầu hoạt động
            counter->total_ms += (now - counter->last_on_ms); // Cộng thời gian hoạt động vào tổng thời gian
            counter->last_on_ms = 0; // Reset thời điểm bắt đầu hoạt động
            op_time_counter_save(counter); // Lưu tổng thời gian hoạt động vào EEPROM
        }
    }

    // Nếu đang ON, lưu định kỳ mỗi 60s để tránh mất dữ liệu khi reset ESP
    if (counter->is_operating && (now - counter->last_save_ms > 60000)) {
        counter->total_ms += (now - counter->last_on_ms); // Cộng thời gian hoạt động vào tổng thời gian
        counter->last_on_ms = now; // Cập nhật thời điểm bắt đầu hoạt động
        op_time_counter_save(counter); // Lưu tổng thời gian hoạt động vào EEPROM
    }

    counter->was_operating = counter->is_operating; // Cập nhật trạng thái trước đó
}

// Hàm reset thời gian hoạt động của thiết bị
void op_time_counter_reset(OperatingTimeCounter* counter) {
    counter->total_ms = 0; // Reset tổng thời gian hoạt động về 0
    counter->last_on_ms = 0; // Reset thời điểm bắt đầu hoạt động
    op_time_counter_save(counter); // Lưu thay đổi vào EEPROM
}

// Hàm lấy tổng thời gian hoạt động (ms)
uint64_t op_time_counter_get_ms(OperatingTimeCounter* counter) {
    // Nếu thiết bị đang hoạt động, cộng thời gian hiện tại vào tổng thời gian
    if (counter->is_operating && counter->last_on_ms > 0) {
        return counter->total_ms + (millis() - counter->last_on_ms);
    }
    return counter->total_ms; // Nếu không hoạt động, trả về tổng thời gian đã lưu
}

// Hàm lấy tổng thời gian hoạt động dưới dạng chuỗi định dạng (HH:MM:SS)
void op_time_counter_get_formatted(OperatingTimeCounter* counter, char* buf, size_t len) {
    uint64_t ms = op_time_counter_get_ms(counter); // Lấy tổng thời gian hoạt động (ms)
    uint32_t sec = ms / 1000; // Chuyển đổi sang giây
    uint32_t h = sec / 3600; // Tính số giờ
    uint32_t m = (sec % 3600) / 60; // Tính số phút
    uint32_t s = sec % 60; // Tính số giây
    snprintf(buf, len, "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s); // Ghi chuỗi định dạng vào buffer
}