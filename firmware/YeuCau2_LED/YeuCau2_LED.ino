#include <Arduino.h>

// De 2 - Yeu cau 2: quet 3 LED, moi trang thai keo dai 400 ms.
// Module den: R -> GPIO22, Y -> GPIO21, G -> GPIO18, GND -> GND.
constexpr uint8_t LED_DO = 22;
constexpr uint8_t LED_VANG = 21;
constexpr uint8_t LED_XANH = 18;
constexpr unsigned long THOI_GIAN_TRANG_THAI_MS = 400;

uint8_t trangThai = 0;
unsigned long lanChuyenTrangThaiMs = 0;

void hienThiTrangThai(uint8_t giaTri) {
  // 0: tat het; 1: do; 2: vang; 3: xanh; 4: sang ca ba.
  digitalWrite(LED_DO, (giaTri == 1 || giaTri == 4) ? HIGH : LOW);
  digitalWrite(LED_VANG, (giaTri == 2 || giaTri == 4) ? HIGH : LOW);
  digitalWrite(LED_XANH, (giaTri == 3 || giaTri == 4) ? HIGH : LOW);
}

void setup() {
  pinMode(LED_DO, OUTPUT);
  pinMode(LED_VANG, OUTPUT);
  pinMode(LED_XANH, OUTPUT);

  hienThiTrangThai(trangThai);
  lanChuyenTrangThaiMs = millis();
}

void loop() {
  const unsigned long bayGioMs = millis();

  // Khong dung delay(), de co the them Blynk va cam bien ve sau.
  if (bayGioMs - lanChuyenTrangThaiMs >= THOI_GIAN_TRANG_THAI_MS) {
    lanChuyenTrangThaiMs = bayGioMs;
    trangThai = (trangThai + 1) % 5;
    hienThiTrangThai(trangThai);
  }
}
