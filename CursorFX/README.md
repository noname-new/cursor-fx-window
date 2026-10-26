# CursorFX for Windows

Bộ mã nguồn CursorFX được đóng gói từ nội dung bạn cung cấp thành cấu trúc dự án đầy đủ.

## Yêu cầu
- Windows 10/11
- MinGW-w64 g++ (C++17; khuyến nghị bản 14.2.0)
- `g++` có trong `PATH`

## Build
1. Giải nén ZIP.
2. Mở Command Prompt tại thư mục `CursorFX`.
3. Chạy `build.bat`.
4. Nếu build thành công, chạy `CursorFX.exe`.

Có thể dùng CMake bằng `CMakeLists.txt` nếu đã cài CMake và toolchain MinGW.

## Phím tắt
- `Ctrl+Alt+S`: mở Settings
- `Ctrl+Alt+Q`: thoát

## Lưu ý an toàn và giới hạn hiện tại
- `HideCursor` mặc định tắt. Nên giữ tắt cho đến khi đã kiểm tra chức năng khôi phục trên máy của bạn.
- Cơ chế backup cursor trong mã hiện tại lưu handle trong RAM; marker file không lưu được handle để phục hồi sau khi tiến trình chết hoặc Windows khởi động lại. Vì vậy không thể cam kết tự khôi phục trong mọi tình huống. Nếu cursor biến mất, thử đăng xuất/đăng nhập hoặc khởi động lại Windows.
- `BlendMode=1` chưa được triển khai; hiện tại chỉ có chế độ hòa trộn mặc định.
- UI Automation phụ thuộc vào ứng dụng đích; một số trang web có thể trả về phần tử cha thay vì phần tử tương tác.
- CMakeLists.txt và build.bat liên kết các thư viện Windows cần thiết.

## Cấu trúc
- `src/`: mã C++
- `res/cursorfx.ini`: cấu hình mặc định tham khảo
- `build.bat`: build nhanh bằng g++
- `CMakeLists.txt`: cấu hình CMake

Lưu ý: gói này đã được sắp xếp thành các file riêng và sửa một include còn thiếu cùng cách đọc số thực cho tương thích MinGW. Chưa thể xác nhận build thực tế trên máy Windows trong môi trường này.
