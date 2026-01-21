#pragma once // Đảm bảo file header chỉ được biên dịch một lần, tránh lỗi lặp khai báo

#include <stdint.h> // Khai báo các kiểu dữ liệu nguyên thủy như uint16_t, uint64_t
#include <stdbool.h> // Khai báo kiểu dữ liệu boolean (true/false)
#include <stddef.h> // Khai báo các định nghĩa kích thước như size_t

#ifdef __cplusplus
extern "C" { // Đảm bảo các hàm trong file này có thể được gọi từ code C++ mà không bị lỗi tên
#endif

// Struct quản lý thời gian hoạt động của thiết bị
typedef struct {
    uint16_t eeprom_addr; // Địa chỉ EEPROM nơi lưu dữ liệu thời gian hoạt động
    uint64_t total_ms;    // Tổng thời gian hoạt động (ms), được lưu lâu dài vào EEPROM
    uint32_t last_on_ms;  // Thời điểm thiết bị bắt đầu hoạt động (ms từ khi ESP khởi động)
    bool is_operating;    // Trạng thái hiện tại của thiết bị (đang hoạt động hay không)
    bool was_operating;   // Trạng thái trước đó của thiết bị (để phát hiện thay đổi)
    uint32_t last_save_ms; // Thời điểm cuối cùng lưu dữ liệu vào EEPROM (ms từ khi ESP khởi động)
} OperatingTimeCounter;

// Hàm khởi tạo bộ đếm thời gian hoạt động
void op_time_counter_init(OperatingTimeCounter* counter, uint16_t eeprom_addr);
// - Đọc dữ liệu từ EEPROM để khởi tạo giá trị `total_ms`
// - Đặt các giá trị ban đầu cho các trường khác trong struct

// Hàm cập nhật trạng thái hoạt động của thiết bị
void op_time_counter_update(OperatingTimeCounter* counter, bool is_operating);
// - Kiểm tra trạng thái hiện tại (`is_operating`) và trạng thái trước đó (`was_operating`)
// - Nếu chuyển từ ON sang OFF, tính toán thời gian hoạt động và lưu vào `total_ms`
// - Nếu đang ON, lưu định kỳ vào EEPROM để tránh mất dữ liệu khi reset ESP

// Hàm reset thời gian hoạt động của thiết bị
void op_time_counter_reset(OperatingTimeCounter* counter);
// - Đặt `total_ms` về 0 và ghi lại vào EEPROM
// - Dùng khi cần reset thời gian hoạt động của thiết bị

// Hàm lấy tổng thời gian hoạt động (ms)
uint64_t op_time_counter_get_ms(OperatingTimeCounter* counter);
// - Trả về giá trị `total_ms` (tổng thời gian hoạt động của thiết bị)

void op_time_counter_get_formatted(OperatingTimeCounter* counter, char* buf, size_t len);
// - Chuyển đổi `total_ms` thành chuỗi định dạng (ví dụ: "HH:MM:SS")
// - Ghi chuỗi vào buffer `buf` với độ dài tối đa `len`

#ifdef __cplusplus
}
#endif // Kết thúc khối extern "C"