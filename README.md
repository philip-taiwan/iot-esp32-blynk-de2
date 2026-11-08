# Bài nộp IoT ESP32 Blynk Đề 2

Mã nguồn thực hiện 5 yêu cầu của bài kiểm tra môn **Thiết kế và Lập trình ứng dụng IoT**, thời gian 120 phút. Phần điều khiển LED sử dụng máy trạng thái 400 ms; cảnh báo HC-SR04 được xử lý độc lập với kết nối mạng.

Repository công khai: <https://github.com/philip-taiwan/iot-esp32-blynk-de2>. Mã nguồn công khai chỉ chứa cấu hình mẫu; token và mật khẩu thực được giữ trong tệp cục bộ đã loại khỏi Git.

## Tệp chính

- `firmware/IoT_ESP32/IoT_ESP32.ino`: chương trình ESP32 tích hợp.
- `firmware/YeuCau2_LED/YeuCau2_LED.ino`: bản quét 3 LED độc lập đã nạp thành công trong buổi thực hành.
- `firmware/IoT_ESP32/Controller.h`: thuật toán điều khiển dùng chung cho firmware và kiểm thử.
- `firmware/IoT_ESP32/ButtonDebouncer.h`: chống dội và nhận mức nhấn của module nút active HIGH hoặc active LOW.
- `firmware/IoT_ESP32/secrets.example.h`: cấu hình mẫu không chứa mật khẩu hoặc token thực.
- `tests/controller_tests.cpp`: kiểm thử logic C++ trên máy tính.
- `tests/run_tests.cmd`: biên dịch và chạy kiểm thử bằng MSVC.
- `validation.json`: kết quả kiểm tra biên dịch và các giới hạn kiểm chứng.

## 1 Đấu nối phần cứng

| Thiết bị | ESP32 | Cách đấu |
|---|---|---|
| LED đỏ LED1 chân R | GPIO22 | Module LED kích mức HIGH |
| LED vàng LED2 chân Y | GPIO21 | Module LED kích mức HIGH |
| LED xanh LED3 chân G | GPIO18 | G trên module là xanh, không phải mass |
| Module LED chân GND | GND | Mass chung với ESP32 |
| BUTTON1 chân OUT hoặc S | GPIO19 | Module nút nhấn 3 chân |
| BUTTON2 chân OUT hoặc S | GPIO5 | Module nút nhấn 3 chân |
| VCC của hai module nút | 3V3 | Cấp 3,3V để OUT tương thích mức điện áp ESP32 |
| GND của hai module nút | GND | Mass chung với ESP32 |
| HC-SR04 VCC | 5V | Nguồn 5V và mass chung |
| HC-SR04 GND | GND | Mass chung |
| HC-SR04 TRIG | GPIO23 | Xung kích 10 micro giây |
| HC-SR04 ECHO | GPIO27 | Qua cầu chia áp 1 kΩ và 2 kΩ |

Đường ECHO: **ECHO → điện trở 1 kΩ → GPIO27 → điện trở 2 kΩ → GND**. Mức 5V của ECHO được chia xuống khoảng 3,33V. Không đấu trực tiếp ECHO 5V vào ESP32. Nếu dùng LED rời, mỗi LED cần điện trở hạn dòng phù hợp; kiểm tra điện trở có sẵn trên module đang dùng.

GPIO5 là chân cấu hình khởi động của ESP32: để BUTTON2 ở trạng thái thả khi reset hoặc nạp chương trình.

## 2 Máy trạng thái LED

| Trạng thái | LED1 đỏ | LED2 vàng | LED3 xanh | Thời gian |
|---|---|---|---|---|
| 0 | Tắt | Tắt | Tắt | 400 ms |
| 1 | Sáng | Tắt | Tắt | 400 ms |
| 2 | Tắt | Sáng | Tắt | 400 ms |
| 3 | Tắt | Tắt | Sáng | 400 ms |
| 4 | Sáng | Sáng | Sáng | 400 ms |

Khởi động tại trạng thái 0, sau đó lặp 0 → 1 → 2 → 3 → 4 → 0. Thuật toán dùng hiệu thời gian `millis()` với số nguyên không dấu, hoạt động qua thời điểm tràn bộ đếm. Không dùng `delay(400)`.

## 3 Blynk trên web và app

Template được cấu hình: **IoT ESP32 De 2**, ID **TMPL6JTpyMoNy**, phần cứng ESP32, kết nối Wi-Fi.

| Virtual pin | Tên gợi ý | Kiểu và miền giá trị | Widget web và app |
|---|---|---|---|
| V0 | Den vang xanh | Integer 0..1 | Switch hoặc Button chế độ Switch |
| V1 | Khoang cach | Double 0..400, cm | Gauge hoặc Labeled Value |
| V2 | Bao dong do | Integer 0..255 | LED màu đỏ |
| V3 | Khoa uu tien | Integer 0..1 | LED trạng thái ưu tiên |
| V4 | Dieu khien LED do | Integer 0..1 | Switch LED1 thủ công |
| V5 | Che do tu dong | Integer 0..1 | Switch chọn quét tự động |

**V0** điều khiển đồng thời LED vàng và xanh: ON bật cả hai, OFF tắt cả hai. Khi người dùng thay đổi V0, firmware chuyển sang chế độ thủ công để chuỗi tự động không ghi đè lệnh. Giá trị phản hồi V0 bằng 1 khi cả hai LED thực tế cùng sáng.

**V2** phản ánh điều kiện cảnh báo khoảng cách, không phản ánh các lần LED đỏ sáng bình thường trong chuỗi tự động. V2 = 255 nếu khoảng cách hợp lệ gần nhất <15 cm; V2 = 0 nếu khoảng cách >=15 cm.

**V4** lưu lệnh LED đỏ thủ công. Khi ưu tiên nguy hiểm đang bật, firmware từ chối lệnh mới trên V4 và trả lại lệnh đã lưu. Đầu ra thực tế của LED đỏ vẫn do cảnh báo quyết định.

**V5** ON chọn quét tự động, OFF chọn thủ công. Dữ liệu được gửi mỗi giây, tối đa 6 Virtual Pin mỗi chu kỳ. Không đồng bộ lại lệnh cũ từ cloud khi kết nối lại; thiết bị gửi trạng thái cục bộ hiện tại lên giao diện.

Web Dashboard và Mobile Dashboard là hai bố cục riêng. Sau khi tạo widget web, mở Blynk IoT trên điện thoại, bật Developer Mode và thêm các widget với đúng datastream trong bảng trên.

## 4 Cảnh báo và thứ tự ưu tiên

| Điều kiện khoảng cách hợp lệ | Hành vi |
|---|---|
| d <10 cm | LED đỏ sáng; vàng và xanh tắt; dừng tiến trình quét; khóa lệnh V4; ghi log khi mới đi vào vùng này |
| 10 cm ≤ d <15 cm | LED đỏ sáng; V2 báo động; các chức năng khác theo chế độ được chọn |
| d ≥15 cm | Không có cảnh báo khoảng cách; xuất LED theo chế độ tự động hoặc thủ công |
| Không có echo hoặc kết quả ngoài miền đo | Không gửi giá trị 0 giả; giữ cảnh báo và khóa hiện tại đến lần đo hợp lệ tiếp theo |

Thứ tự quyết định đầu ra là **nguy hiểm <10 cm → cảnh báo <15 cm → chế độ tự động hoặc thủ công**. Khi rời vùng <10 cm bằng một phép đo hợp lệ, hệ thống mở khóa và tiếp tục phần thời gian 400 ms còn lại của trạng thái bị dừng nếu chế độ tự động vẫn được chọn. Có thể đổi V5 trong lúc nguy hiểm, nhưng điều đó không hủy ưu tiên cảnh báo.

HC-SR04 được lấy mẫu mỗi 100 ms, chờ ECHO tối đa 25 ms. Cảnh báo cập nhật sau phép đo kế tiếp, không chờ hết một bước LED. Khoảng thời gian phát hiện theo thiết kế khoảng 100–125 ms, chưa phải số đo thực nghiệm trên mạch tích hợp. Mất ECHO khi hệ thống chưa từng đo được khoảng cách không tự tạo giá trị khoảng cách hoặc một sự kiện nguy hiểm giả.

Blynk và Wi-Fi chạy trong task FreeRTOS riêng trên core 0 của ESP32 hai nhân. Vòng lặp phần cứng tiếp tục đọc cảm biến và áp dụng đầu ra trên core 1; thao tác DNS, socket hoặc kết nối cloud chậm không chặn vòng lặp này. Trạng thái chia sẻ được bảo vệ bằng critical section ngắn.

## 5 Hai nút nhấn vật lý và log

Đề chỉ nêu chân nối của hai nút, chưa quy định chức năng. Bài làm chọn:

- BUTTON1: chuyển giữa tự động và thủ công.
- BUTTON2: bật hoặc tắt đồng thời LED vàng và xanh, đồng thời chọn thủ công.

Cả hai nút chống dội bằng `millis()` trong 30 ms. Khi khởi động, firmware lấy mức OUT lúc nút được thả làm mức nghỉ, rồi coi mức đối lập là nhấn; cách này hỗ trợ module active HIGH và active LOW. **Thả cả hai nút trước khi reset hoặc cấp nguồn** để hiệu chuẩn đúng. GPIO được cấu hình INPUT_PULLUP; module có điện trở kéo xuống bên trong vẫn có thể tạo mức nghỉ LOW. Serial Monitor in mức nghỉ và mức tác động đã nhận. Ưu tiên cảnh báo được áp dụng sau lệnh nút nhấn.

Mỗi lần đi vào vùng <10 cm tạo một bản ghi với thời gian tính từ lúc ESP32 reset, đơn vị mili giây:

```text
[CRITICAL] uptime_ms=12530 distance_cm=7.82 auto_paused=1 red_manual_locked=1
```

Dòng trên là **ví dụ định dạng**, không phải dữ liệu đo được của bài thực hành. Bộ đệm vòng lưu 16 sự kiện gần nhất trong RAM. Gửi ký tự `L` trong Serial Monitor 115200 baud để in bộ đệm. Dữ liệu RAM mất sau khi reset; dấu thời gian là uptime, không phải giờ lịch.

Để kiểm tra dây tín hiệu nút, gửi ký tự `D` trong Serial Monitor 115200 baud. Firmware in mức đọc GPIO19/GPIO5 và mức nghỉ đã nhận mỗi 250 ms; gửi `D` lần nữa để dừng. Hai module cần được thả khi reset; chương trình chờ 100 ms cho đầu ra ổn định trước khi nhận mức nghỉ.

## 6 Cấu hình và nạp

Để tạm bỏ qua hai nút vật lý khi làm phần Blynk và cảm biến, thêm `#define IOT_ENABLE_BUTTONS 0` vào `secrets.h` cục bộ. Cấu hình mẫu mặc định là `1` để sử dụng nút khi phần cứng đã sẵn sàng.

1. Cài Arduino IDE, core **esp32 by Espressif Systems 3.3.12** và thư viện **Blynk 1.4.0**.
2. Mở `firmware/IoT_ESP32/IoT_ESP32.ino`.
3. Sao chép `secrets.example.h` thành `secrets.h` trong cùng thư mục.
4. Điền Auth Token của thiết bị và tên/mật khẩu Wi-Fi vào `secrets.h`, rồi đổi `IOT_ENABLE_CLOUD` thành `1`. Template ID và tên đã có trong file mẫu. Không đưa token hoặc mật khẩu vào báo cáo hay GitHub.
5. Chọn **ESP32 Dev Module** hoặc cấu hình **DOIT ESP32 DEVKIT V1** tương thích với board đang dùng, chọn COM tương ứng rồi Upload.
6. Nếu IDE báo `Wrong boot mode`, giữ BOOT trong lúc kết nối để nạp; khi xuất hiện `Writing at...` thì thả BOOT.

Mặc định `IOT_ENABLE_CLOUD = 0`: chương trình vẫn biên dịch và chạy phần cục bộ mà không cố kết nối bằng thông tin mẫu. Không cần tạo `secrets.h` để biên dịch kiểm tra.

Khi mới đấu LED và hai nút, đặt `IOT_ENABLE_SENSOR = 0` trong `secrets.h` cục bộ để không đọc chân ECHO đang chưa nối. Khi HC-SR04 và cầu chia áp đã đấu xong, đổi giá trị này thành `1` để bật đo và cảnh báo; cấu hình mẫu công khai mặc định bật cảm biến. Serial Monitor in trạng thái cảm biến lúc khởi động và các dòng `[BUTTON1] mode=AUTO/MANUAL`, `[BUTTON2] pair=ON/OFF` sau mỗi lần nhấn đã chống dội.

**Giới hạn dữ liệu lỗi:** V1 không được cập nhật khi không có phép đo hợp lệ, vì vậy Blynk có thể hiển thị giá trị hợp lệ cũ. Khi thử nghiệm cần theo dõi trạng thái kết nối và phép đo thực tế; việc giữ cảnh báo cũ là chủ ý để một lần mất echo không tự giải phóng khóa.

## 7 Kiểm chứng và phạm vi thực nghiệm

Chuỗi LED của yêu cầu 2 đã được nạp thành công vào ESP32 qua COM3 bằng sketch riêng `YeuCau2_LED` trong buổi thực hành. Bản sao nguyên vẹn nằm tại `firmware/YeuCau2_LED/YeuCau2_LED.ino`. Hình ảnh thực tế do người thực hiện cung cấp cho thấy bộ LED đã đấu nối và sáng. Firmware tích hợp trong repository này được kiểm tra biên dịch và kiểm thử logic; chưa xác nhận đo HC-SR04, hai nút nhấn và điều khiển Blynk trên phần cứng tích hợp.

Chạy kiểm thử trên Windows có MSVC Build Tools:

```bat
tests\run_tests.cmd
```

Hoặc trên môi trường có GCC/Clang:

```sh
g++ -std=c++17 tests/controller_tests.cpp -o controller_tests
./controller_tests
```

Các nhóm kiểm thử: chuỗi LED và biên 400 ms; ngưỡng 9,9/10/14,9/15 cm; ưu tiên cảnh báo; dừng và phục hồi pha quét; mất echo; từ chối lệnh đỏ khi khóa; tràn `millis()`; bắt kịp pha khi vòng lặp bị trễ; nút active LOW; nút active HIGH. Hai nhóm nút kiểm tra dội tiếp điểm, chỉ tạo một sự kiện khi giữ nút, không tạo sự kiện nhấn lúc thả và xử lý tràn thời gian.

Kịch bản kiểm tra trên mạch đầy đủ: giữ vật ở >=15 cm, 10–<15 cm và <10 cm; thử V4 trong khi nguy hiểm; đưa vật ra xa; thử V0/V5 và hai nút; ngắt Wi-Fi; thử mất echo; kiểm tra log và phản hồi giao diện.

## Tài liệu kỹ thuật

- [Arduino Blink Without Delay](https://docs.arduino.cc/built-in-examples/digital/BlinkWithoutDelay/)
- [Blynk Virtual Pins](https://docs.blynk.io/en/blynk-library-firmware-api/virtual-pins)
- [Blynk Timers](https://docs.blynk.io/en/blynk-library-firmware-api/blynk-timer)
- [Blynk Connection Management](https://docs.blynk.io/en/blynk-library-firmware-api/connection-management)
- [Espressif Boot Mode Selection](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html)
