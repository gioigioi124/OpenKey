# Báo cáo Bàn giao Kỹ thuật (Handoff Report - Agent 1: Clarify & Plan / Explorer)

**Dự án**: OpenKey Win32  
**Nhiệm vụ**: Phân tích, thiết kế giải pháp và lập kế hoạch kiểm thử cho tính năng Truy vết Cửa sổ Cha (Parent / Owner Window Tracing) khi mở UserForm / Hộp thoại con  
**Agent thực hiện**: Agent 1 (Explorer & Planner)  
**Người nhận**: Agent 2 (Developer / Worker) & Orchestrator  
**Tệp phân tích đầy đủ**: `c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3\analysis.md`

---

## 1. Observation (Các Quan sát Trực tiếp)

1. **Vị trí thiết lập hook sự kiện cửa sổ**:
   - Tệp: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`
   - Dòng 179-180:
     ```cpp
     hSystemEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
     hTitleEvent = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
     ```
   - Quan sát: Callback `winEventProcCallback` được đăng ký nhận sự kiện chuyển cửa sổ tiền cảnh (`EVENT_SYSTEM_FOREGROUND`) và đổi tiêu đề cửa sổ (`EVENT_OBJECT_NAMECHANGE`).

2. **Cách thức lấy tiêu đề và nhận diện bảng mã hiện tại**:
   - Tệp: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`
   - Dòng 726-741:
     ```cpp
     if (vAutoSwitchCodeTable) {
         string title = OpenKeyHelper::getFrontMostWindowTitleUtf8();
         int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, title);
         if (ruleCode != -1) {
             if (vCodeTable != ruleCode) {
                 AppDelegate::getInstance()->onTableCode(ruleCode);
                 SystemTrayHelper::updateData();
             }
         } else if (vFallbackToUnicode) {
             if (vCodeTable != 0) {
                 AppDelegate::getInstance()->onTableCode(0);
                 SystemTrayHelper::updateData();
             }
         }
         // If ruleCode == -1 and !vFallbackToUnicode, retain current encoding without fallback!
     }
     ```
   - Tệp: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp`
   - Dòng 158-169:
     ```cpp
     string OpenKeyHelper::getFrontMostWindowTitleUtf8() {
         HWND hForeground = GetForegroundWindow();
         if (!hForeground) return "";
         WCHAR titleBuf[1024] = { 0 };
         int len = GetWindowTextW(hForeground, titleBuf, 1024);
         if (len <= 0) return "";
         int size_needed = WideCharToMultiByte(CP_UTF8, 0, titleBuf, len, NULL, 0, NULL, NULL);
         if (size_needed <= 0) return "";
         std::string strTo(size_needed, 0);
         WideCharToMultiByte(CP_UTF8, 0, titleBuf, len, &strTo[0], size_needed, NULL, NULL);
         return strTo;
     }
     ```
   - Quan sát:
     - `OpenKeyHelper::getFrontMostWindowTitleUtf8()` chỉ gọi `GetWindowTextW(GetForegroundWindow(), ...)` để lấy duy nhất tiêu đề của cửa sổ tiền cảnh hiện hành.
     - Khi mở UserForm trong Excel (lớp `ThunderDFrame`, tiêu đề `"UserForm1"` hoặc tương tự), `title` là `"UserForm1"`. Tiêu đề này không chứa tên file Excel cha (`"a"`).
     - Kết quả `ProcessRuleHelper::getCodeTableForProcessAndTitle("excel.exe", "UserForm1")` trả về `-1`.
     - Nếu `vFallbackToUnicode == 1`, luồng thực thi lập tức nhảy vào nhánh `else if (vFallbackToUnicode)` và ép bảng mã về `0` (Unicode), phá vỡ cấu hình bảng mã TCVN3 của file Excel cha!

3. **Thuật toán so khớp quy tắc hiện tại**:
   - Tệp: `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp`
   - Dòng 240-268:
     - Pass 1: So khớp cả `processName` và `titlePattern`.
     - Pass 2: So khớp chỉ `titlePattern`.
     - Pass 3: So khớp chỉ `processName` (khi `titlePattern.empty()`).
   - Quan sát: Không có cơ chế nhận diện phân cấp cha/con (Hierarchy awareness). Nếu có một quy tắc tiến trình chung (như `excel.exe = UNICODE`) cùng với quy tắc file (như `excel.exe[a] = TCVN3`), việc gọi trực tiếp trên UserForm sẽ bị Pass 3 bắt trước khi kịp truy vết tên file của cửa sổ cha.

---

## 2. Logic Chain (Chuỗi Lập luận Kỹ thuật)

1. Từ **Observation 2**: Khi UserForm được mở, cửa sổ tiền cảnh là UserForm chứ không phải bảng tính Excel. Tiêu đề của UserForm thường không phản ánh tên tệp đang mở trong Excel.
2. Từ kiến trúc Win32:
   - Các hộp thoại VBA UserForm, popup dialog hay modal message box được tạo ra với quan hệ sở hữu (Owner/Owned Window) gắn liền với cửa sổ ứng dụng gốc (`XLMAIN`) và cùng chia sẻ chung một Process ID (`excel.exe`).
   - Win32 API cung cấp các hàm chuyên dụng:
     - `GetAncestor(hwnd, GA_ROOTOWNER)`: Truy ngược chuỗi cửa sổ cha và sở hữu lên tận cửa sổ gốc cao nhất.
     - `GetWindow(hwnd, GW_OWNER)`: Lấy handle cửa sổ sở hữu trực tiếp.
     - `GetWindowThreadProcessId(hwnd, &pid)`: Xác thực Process ID của cửa sổ để đảm bảo tính an toàn.
3. Để giải quyết triệt để lỗi nhảy bảng mã mà không phá vỡ tính năng hiện có:
   - **Thứ tự ưu tiên 1 (Quy tắc riêng của con)**: Nếu UserForm được cấu hình quy tắc riêng trong `process_rules.ini` (ví dụ: `excel.exe[FormNhapLieu] = VNI`), quy tắc này phải được áp dụng ngay lập tức mà không bị quy tắc của cha ghi đè.
   - **Thứ tự ưu tiên 2 (Kế thừa từ cha)**: Nếu cửa sổ con không có quy tắc riêng, hệ thống tự động truy vết `hOwner` (`GW_OWNER`) và `hRootOwner` (`GA_ROOTOWNER`) trong cùng Process ID. Nếu tiêu đề của cửa sổ cha (ví dụ `XLMAIN` có tiêu đề chứa `"a"`) khớp với quy tắc `excel.exe[a] = TCVN3`, áp dụng ngay bảng mã của cha cho UserForm.
   - **Thứ tự ưu tiên 3 (Quy tắc tiến trình chung)**: Nếu không có quy tắc theo tiêu đề cho cả con và cha, kiểm tra quy tắc tiến trình chung (`s.exe = TCVN3`).
   - **Thứ tự ưu tiên 4 (Fallback về Unicode)**: Chỉ khi cả con, cha, lẫn tiến trình đều không khớp bất kỳ quy tắc nào, và `vFallbackToUnicode == 1`, mới cho phép chuyển về Unicode.
4. Từ **Observation 1 & 2**: Tất cả các thao tác chuyển bảng mã phải được thực hiện thông qua `AppDelegate::getInstance()->onTableCode(ruleCode)` và `SystemTrayHelper::updateData()`. Điều này bảo đảm nguyên tắc **Single Source of Truth**: Registry, Main Dialog, Tray menu và cơ chế gõ tắt On-Demand Lazy JIT (`onTableCodeChange()`) luôn luôn đồng bộ 100%.

---

## 3. Caveats (Các Điểm Giới hạn & Lưu ý Cần Chú ý)

1. **Ranh giới Tiến trình (Cross-Process Boundaries)**:
   - Một số cửa sổ đặc thù (như cửa sổ Shell, ApplicationFrameHost, hoặc OLE in-place activation) có thể có owner thuộc tiến trình khác. Hệ thống phải kiểm tra nghiêm ngặt `targetPid == ownerPid` và `targetPid != 0`. Nếu PID khác nhau, lập tức dừng duyệt.
2. **Cửa sổ Rỗng / Không Tiêu đề**:
   - Khi `childTitle` rỗng (`""`), không được xem là đã khớp quy tắc (tránh kích hoạt nhầm Pass 3), mà phải tiếp tục chuyển tiếp sang Bước truy vết cửa sổ cha.
3. **Phòng chống Treo ứng dụng & Vòng lặp**:
   - Giới hạn độ sâu duyệt chuỗi `GW_OWNER` tối đa 10 bước.
   - Dừng ngay nếu handle là `NULL`, bằng chính nó (`cur == next`), `!IsWindow(next)`, hoặc là `GetDesktopWindow()`.
   - `GetWindowTextW` đối với cửa sổ ngoài tiến trình đọc trực tiếp từ cấu trúc nội bộ của `USER32`, không gửi message đồng bộ `WM_GETTEXT`, nên không bao giờ gây treo/deadlock khi ứng dụng mục tiêu bị đơ.
4. **Môi trường Biên dịch**:
   - Kịch bản `build.bat` yêu cầu `clang++` và `windres.exe` có sẵn trong PATH để tạo tệp thực thi `OpenKey.exe`. Agent 2 cần đảm bảo môi trường dòng lệnh có thể truy cập được các công cụ này khi chạy build.

---

## 4. Conclusion (Kết luận & Đề xuất Hành động)

1. **Đánh giá Cuối cùng**: Giải pháp truy vết cửa sổ cha hai lớp (`GW_OWNER` và `GA_ROOTOWNER`) kết hợp kiểm tra PID là giải pháp tối ưu nhất, xử lý triệt để bài toán UserForm/Dialog trong Excel và mọi ứng dụng Win32 khác, thỏa mãn 100% các tiêu chí nghiệm thu (Acceptance Criteria).
2. **Kế hoạch Triển khai Cụ thể cho Agent 2**:
   - **Tệp 1**: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.h`: Khai báo `getWindowTitleUtf8(HWND hwnd)` và `getProcessRootOwner(HWND hwnd)`.
   - **Tệp 2**: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp`: Triển khai 2 hàm trên và chuyển `getFrontMostWindowTitleUtf8()` gọi về `getWindowTitleUtf8(GetForegroundWindow())`.
   - **Tệp 3**: `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.h`: Khai báo `getCodeTableForTitleOnly(exe, title)` và `getCodeTableForWindow(hwnd, exe)`.
   - **Tệp 4**: `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp`: Triển khai thuật toán so khớp ưu tiên phân cấp (Con -> Cha -> Tiến trình).
   - **Tệp 5**: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`: Cập nhật `winEventProcCallback` gọi `ProcessRuleHelper::getCodeTableForWindow(hActiveWnd, exe)`.
   - Chi tiết mã nguồn từng dòng được cung cấp đầy đủ trong mục 5 của tệp `analysis.md`.
3. **Biên dịch & Kiểm thử**: Chạy `build.bat` biên dịch `OpenKey.exe` và thực hiện kiểm thử theo Ma trận Kiểm thử 14 kịch bản (TC-01 đến TC-14).

---

## 5. Verification Method (Phương pháp Kiểm chứng Độc lập)

1. **Kiểm chứng Tĩnh (Static Inspection)**:
   - Kiểm tra `OpenKeyHelper.h/cpp`, `ProcessRuleHelper.h/cpp`, `OpenKey.cpp` có đầy đủ các hàm được mô tả trong kế hoạch.
   - Kiểm tra không có rò rỉ bộ nhớ, không có vòng lặp không giới hạn, không có bỏ qua kiểm tra PID.
2. **Kiểm chứng Biên dịch (Build Verification)**:
   - Chạy lệnh:
     ```cmd
     build.bat
     ```
   - Điều kiện đạt: Exit code = 0, tệp `OpenKey.exe` được sinh ra thành công tại thư mục gốc và thư mục `Sources/OpenKey/win32/OpenKey/OpenKey/`.
3. **Kiểm chứng Động theo Ma trận Kiểm thử (Test Matrix Execution)**:
   - **TC-01 (Mở UserForm trong Excel `a.xlsx` với `fallback_to_unicode = 1`)**: Bảng mã phải duy trì TCVN3, không được nhảy về Unicode.
   - **TC-02 (Gõ tiếng Việt trong UserForm)**: Xuất đúng ký tự tiếng Việt bảng mã TCVN3 (ABC).
   - **TC-04 (UserForm có quy tắc riêng)**: Mở UserForm `FormVNI`, bảng mã chuyển sang VNI ngay lập tức.
   - **TC-06 (Hộp thoại tiêu đề rỗng)**: Mở popup không tiêu đề, bảng mã kế thừa đúng từ cha.
   - **TC-08 & TC-10 (Fallback về Unicode)**: Khi rời sang file Excel không có quy tắc hoặc sang Notepad, bảng mã tự động fallback về Unicode khi `fallback_to_unicode = 1`.
   - **TC-12 (Khóa tính năng)**: Bấm `Ctrl + Shift + F12`, mở UserForm, bảng mã không tự động thay đổi.
4. **Điều kiện Hủy bỏ (Invalidation Conditions)**:
   - Nếu UserForm trong file `a.xlsx` bị nhảy về Unicode khi `fallback_to_unicode = 1` -> Thất bại.
   - Nếu UserForm có quy tắc riêng bị quy tắc của cha ghi đè -> Thất bại.
   - Nếu xảy ra crash hoặc đơ giao diện khi đóng mở UserForm liên tục -> Thất bại.
