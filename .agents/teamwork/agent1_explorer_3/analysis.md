# Báo cáo Phân tích Chuyên sâu & Kế hoạch Kỹ thuật (Agent 1: Explorer & Planner)
## Tính năng: Tự động Kiểm tra Cửa sổ Cha (Parent / Owner Window Tracing) cho UserForm & Hộp thoại con trong OpenKey Win32

**Tác giả**: Agent 1 (Clarify & Plan / Explorer)  
**Ngày thực hiện**: 2026-10-10  
**Thư mục làm việc**: `c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3`  
**Dự án**: OpenKey Win32 (Fork: Auto Encoding & Hotkey)

---

## 1. Tóm tắt Tổng quan & Chẩn đoán Hiện trạng (Executive Summary)

### 1.1. Bối cảnh & Hiện tượng Lỗi
Trong OpenKey Win32, tính năng tự động chuyển bảng mã theo tiến trình và tiêu đề cửa sổ (`ProcessRuleHelper`) cho phép người dùng định nghĩa quy tắc trong `process_rules.ini` (ví dụ: `excel.exe[a] = TCVN3` để khi mở file Excel `a.xlsx` thì tự động chuyển sang bảng mã TCVN3).

Tuy nhiên, khi người dùng làm việc trong Excel (hoặc Word, AutoCAD, phần mềm kế toán) và mở một **VBA UserForm** (hộp thoại nhập liệu viết bằng VBA) hoặc một hộp thoại con/popup (như hộp thoại *Find & Replace*, *Format Cells*, *MsgBox* thông báo):
1. Cửa sổ UserForm / Dialog trở thành cửa sổ tiền cảnh (Foreground Window).
2. Tiêu đề của cửa sổ UserForm này thường là tên Form (ví dụ: `"UserForm1"`, `"FormNhapLieu"`, hoặc tiêu đề rỗng `""`), **không chứa tên file Excel cha (`"a"`)**.
3. Hàm `OpenKeyHelper::getFrontMostWindowTitleUtf8()` hiện tại chỉ đọc tiêu đề của chính cửa sổ tiền cảnh này (`UserForm1`).
4. Khi so khớp với quy tắc trong `process_rules.ini`, tiêu đề `"UserForm1"` **không khớp** với quy tắc `excel.exe[a] = TCVN3`.
5. Kết quả là hàm so khớp trả về `-1` (không khớp quy tắc).
6. **Hậu quả nghiêm trọng**:
   - Nếu tùy chọn **Fallback về Unicode khi rời ứng dụng** đang BẬT (`fallback_to_unicode = 1`): OpenKey lập tức ép bảng mã về **Unicode (`0`)**! Người dùng gõ tiếng Việt trong UserForm bị sai font hoàn toàn (vỡ chữ, không ra đúng font `.VnTime` của bảng tính).
   - Ngay cả khi `fallback_to_unicode = 0`: Nếu UserForm được mở từ một trạng thái chưa kích hoạt TCVN3, hoặc nếu có nhiều file Excel với bảng mã khác nhau đang mở cùng lúc, UserForm không được đồng bộ theo đúng file cha sở hữu nó.

### 1.2. Mục tiêu Giải pháp
Triển khai cơ chế **Truy vết Cửa sổ Cha / Cửa sở hữu (Parent / Owner Window Tracing)**:
1. Khi cửa sổ tiền cảnh được kích hoạt hoặc đổi tiêu đề, trước tiên kiểm tra xem chính cửa sổ đó có quy tắc riêng không (đảm bảo ưu tiên cao nhất cho quy tắc đặc thù của con).
2. Nếu cửa sổ hiện tại không có quy tắc riêng: Tự động truy vết cây cửa sổ sở hữu (`GW_OWNER`) và gốc sở hữu (`GA_ROOTOWNER`) **thuộc cùng Process ID (PID)** để lấy tiêu đề của cửa sổ ứng dụng/file cha (ví dụ `XLMAIN` của `a.xlsx`).
3. Nếu cửa sổ cha khớp quy tắc trong `process_rules.ini`, áp dụng ngay bảng mã của cửa sổ cha cho cửa sổ con.
4. **Chỉ fallback về Unicode** khi và chỉ khi cả cửa sổ con lẫn cửa sổ cha đều không khớp bất kỳ quy tắc nào (và `fallback_to_unicode == 1`).
5. Đảm bảo an toàn tuyệt đối: Không vượt ranh giới tiến trình (Cross-process isolation), ngăn chặn vòng lặp vô hạn, không rò rỉ tài nguyên, độ trễ cực thấp (< 0.05ms) không gây giật lag luồng phím.

---

## 2. Khảo sát Mã nguồn Hiện tại & Điểm nghẽn Kỹ thuật (Codebase Inspection)

### 2.1. Quá trình Bắt Sự kiện Cửa sổ (`OpenKey.cpp`)
- **Vị trí thiết lập hook** (dòng 179-180):
  ```cpp
  hSystemEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
  hTitleEvent = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
  ```
- **Vị trí xử lý callback** (dòng 715-741):
  ```cpp
  VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
      if (dwEvent == EVENT_OBJECT_NAMECHANGE) {
          if (idObject != OBJID_WINDOW || hwnd != GetForegroundWindow())
              return;
      }

      string& exe = OpenKeyHelper::getFrontMostAppExecuteName();
      if (exe.compare("explorer.exe") == 0)
          return;

      // 1. Process & Title recognition rule (only active when not locked)
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
      }
      ...
  }
  ```
**Phát hiện khiếm khuyết**:
- `OpenKeyHelper::getFrontMostWindowTitleUtf8()` chỉ gọi `GetWindowTextW(GetForegroundWindow(), ...)` để lấy duy nhất tiêu đề của cửa sổ tiền cảnh hiện tại.
- Khi UserForm hiển thị, `GetForegroundWindow()` là UserForm (ví dụ class `ThunderDFrame` trong VBA Excel).
- `ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, title)` chỉ so khớp một chuỗi `title` duy nhất. Nếu không khớp, `ruleCode = -1`, dẫn thẳng tới nhánh `else if (vFallbackToUnicode)` làm reset bảng mã về 0 (Unicode).

### 2.2. Cơ chế Lấy Tiêu đề Hiện hành (`OpenKeyHelper.cpp`)
- **Vị trí**: Dòng 158-169 trong `OpenKeyHelper.cpp`:
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
**Phát hiện**:
- Hàm này gắn cứng với `GetForegroundWindow()`, không hỗ trợ truyền handle `HWND` tùy ý để lấy tiêu đề của cửa sổ cha/sở hữu.
- Chưa có hàm tìm cửa sổ cha/gốc sở hữu (`getProcessRootOwner` hoặc tương đương).

### 2.3. Quy trình Phân giải Quy tắc Hiện hành (`ProcessRuleHelper.cpp`)
- **Vị trí**: Dòng 230-268 trong `ProcessRuleHelper.cpp`:
  - Pass 1: So khớp cả `exeName` và `titlePattern`.
  - Pass 2: So khớp chỉ `titlePattern`.
  - Pass 3: So khớp chỉ `exeName` (khi `titlePattern` rỗng).
**Phát hiện**:
- Hàm `getCodeTableForProcessAndTitle` gộp chung cả 3 Pass. Nếu một ứng dụng có quy tắc chung theo tiến trình (ví dụ `excel.exe = UNICODE`) cùng với quy tắc theo file (ví dụ `excel.exe[a] = TCVN3`), việc gọi trực tiếp trên UserForm sẽ bị Pass 3 nuốt mất trước khi kịp truy vết tiêu đề file của cửa sổ cha!
- Cần tách bạch rõ ràng giữa:
  - So khớp theo tiêu đề cụ thể (`getCodeTableForTitleOnly`): Dành cho Pass 1 & Pass 2.
  - So khớp theo tiến trình thuần túy (`getCodeTableForProcess`): Dành cho Pass 3.
  - So khớp phân cấp có tính đến cha/con (`getCodeTableForWindow`): Tích hợp luồng ưu tiên chuẩn.

---

## 3. Phân tích Kiến trúc Win32 & Cơ chế Truy vết Cửa sổ An toàn

### 3.1. Cấu trúc Cửa sổ trong Ứng dụng Win32 & Microsoft Excel
Trong hệ điều hành Windows:
1. **Cửa sổ Top-Level chính (Root Window)**: Trong Excel, đây là cửa sổ lớp `XLMAIN`. Tiêu đề cửa sổ có dạng `Microsoft Excel - a.xlsx` hoặc `a.xlsx - Excel`.
2. **Cửa sổ Con/Sở hữu (Owned / Child Window)**:
   - **VBA UserForm**: Lớp `ThunderDFrame` (hoặc `ThunderXFrame`). Khi được tạo, Excel gán cửa sổ `XLMAIN` làm cửa sổ sở hữu (Owner Window) thông qua `hWndParent` trong `CreateWindowEx` hoặc `SetWindowLongPtr(GWLP_HWNDPARENT)`.
   - **Hộp thoại chuẩn (Dialogs)**: Lớp `#32770` (Find & Replace, Format Cells, Options...). Có Owner là `XLMAIN` hoặc UserForm.
   - **Hộp thoại lồng (Nested Dialogs)**: Ví dụ hàm `MsgBox` được gọi từ bên trong UserForm. `MsgBox` được sở hữu bởi `UserForm`, và `UserForm` lại được sở hữu bởi `XLMAIN`.

### 3.2. So sánh các Win32 API Truy vết Phân cấp Cửa sổ

| Win32 API | Cơ chế hoạt động | Ưu điểm | Hạn chế cần kiểm soát |
|---|---|---|---|
| `GetAncestor(hwnd, GA_ROOTOWNER)` | Duyệt đệ quy lên chuỗi cửa sổ cha (`GetParent`) và chuỗi sở hữu cho đến cửa sổ gốc cao nhất | Trực tiếp nhảy tới cửa sổ gốc `XLMAIN` chỉ bằng 1 lệnh gọi API | Có thể nhảy ra cửa sổ Shell/Desktop hoặc tiến trình khác nếu không kiểm tra PID |
| `GetWindow(hwnd, GW_OWNER)` | Lấy handle của cửa sổ sở hữu trực tiếp (Immediate Owner) | Rất chính xác cho các hộp thoại modal/modeless cấp 1 | Nếu có nhiều cấp lồng (Dialog -> UserForm -> XLMAIN), chỉ lấy được cấp trung gian |
| `GetParent(hwnd)` | Với `WS_CHILD`: trả về cửa sổ cha. Với Top-level: trả về Owner | Hoạt động tốt với control con nhúng | Không phân biệt rõ giữa visual parent và logical owner |
| `GetWindowThreadProcessId(hwnd, &pid)` | Trả về Process ID tạo ra HWND | Cực nhanh, đáng tin cậy 100% | Phải luôn gọi để xác thực PID trước khi tin tưởng HWND cha |

### 3.3. Chiến lược Kết hợp Kép (Hybrid Safe Traversal Strategy)
Để đảm bảo xử lý hoàn hảo mọi trường hợp (từ UserForm trực tiếp đến hộp thoại lồng nhiều cấp):
1. **Bước 1 (Ưu tiên con)**: Kiểm tra tiêu đề của chính `hwnd` tiền cảnh (`childTitle`). Nếu `childTitle` khớp quy tắc tiêu đề cụ thể (`Pass 1` hoặc `Pass 2`), áp dụng ngay! Điều này đảm bảo: Nếu người dùng cấu hình riêng `excel.exe[FormNhapLieu] = VNI`, quy tắc của Form con sẽ được ưu tiên tuyệt đối so với file cha.
2. **Bước 2 (Kiểm tra Owner trực tiếp)**: Lấy `hOwner = GetWindow(hwnd, GW_OWNER)` (hoặc `GetParent` nếu `GW_OWNER` null). Nếu `hOwner` hợp lệ và có cùng PID với `hwnd`: kiểm tra xem `hOwner` có quy tắc tiêu đề không.
3. **Bước 3 (Kiểm tra Root Owner)**: Sử dụng `OpenKeyHelper::getProcessRootOwner(hwnd)` (gọi `GetAncestor(hwnd, GA_ROOTOWNER)` kèm vòng lặp an toàn). Nếu `hRootOwner` hợp lệ và có cùng PID: kiểm tra quy tắc tiêu đề của `hRootOwner`.
4. **Bước 4 (Quy tắc cấp tiến trình)**: Nếu cả con lẫn cha đều không có quy tắc tiêu đề, kiểm tra quy tắc tiến trình chung (`getCodeTableForProcess(exe)`).
5. **Bước 5 (Không khớp)**: Trả về `-1`. Khi đó nhánh `vFallbackToUnicode` mới được phép kích hoạt.

---

## 4. Phân tích Chi tiết Toàn bộ các Trường hợp Biên (Edge Cases Analysis)

### 4.1. Cửa sổ Tiền cảnh đã là Cửa sổ Gốc / Cửa sổ Chính
- **Tình huống**: Người dùng đang thao tác trực tiếp trên trang tính Excel `a.xlsx` (cửa sổ `XLMAIN`).
- **Phân tích**:
  - Tại Bước 1, `childTitle` là `"Microsoft Excel - a.xlsx"`.
  - Khớp ngay quy tắc `excel.exe[a] = TCVN3` tại Bước 1.
  - Hàm trả về `1` ngay lập tức, không thực hiện bất kỳ thao tác truy vết cha nào.
- **Hiệu năng**: Độ phức tạp tối thiểu, không có phép tính dư thừa.

### 4.2. Cửa sổ Con là Modal Dialog hoặc Modeless UserForm
- **Tình huống**:
  - Modal UserForm (`UserForm1.Show vbModal`): Cửa sổ chính Excel bị vô hiệu hóa (disabled), UserForm chiếm tiêu điểm.
  - Modeless UserForm (`UserForm1.Show vbModeless`): Cả cửa sổ chính lẫn UserForm đều có thể nhận click chuột qua lại.
- **Phân tích**:
  - Cả hai dạng UserForm đều có `GW_OWNER` trỏ về cửa sổ `XLMAIN` của tiến trình `excel.exe`.
  - Khi click vào UserForm: Tiêu đề `"UserForm1"` không có quy tắc -> Bước 2/3 truy vết ra `XLMAIN` -> Đọc tiêu đề `a.xlsx` -> Trả về `1` (TCVN3).
  - Bảng mã được giữ nguyên vẹn là TCVN3, không xảy ra hiện tượng nhảy giật sang Unicode.
  - Khi click qua lại giữa bảng tính và Modeless UserForm: Cả 2 cửa sổ đều giải quyết ra cùng một mã `1`, do đó `vCodeTable == ruleCode`, không gọi lệnh đổi bảng mã dư thừa.

### 4.3. Hộp thoại Con có Tiêu đề Rỗng (`""`) hoặc Tiêu đề Hệ thống
- **Tình huống**: Một số thanh công cụ nổi (floating palette), hộp thoại chọn màu, hoặc dialog không đặt Caption (`len == 0`).
- **Phân tích**:
  - `childTitle` là `""`. Bước 1 bỏ qua vì không có tiêu đề.
  - Bước 2/3 truy vết ra `hOwner` / `hRootOwner` (`XLMAIN`), lấy được tiêu đề `a.xlsx` -> Khớp `TCVN3`.
  - Ngăn chặn hoàn toàn lỗi nhảy về Unicode khi mở các dialog không có tiêu đề.

### 4.4. Cửa sổ Con có Quy tắc Riêng vs Không có Quy tắc
- **Tình huống**:
  ```ini
  excel.exe[a] = TCVN3
  excel.exe[FormNhapLieu] = VNI
  ```
- **Phân tích**:
  - Khi mở Form thường (`UserForm1`): Không có quy tắc riêng -> Kế thừa `TCVN3` của cha.
  - Khi mở `FormNhapLieu`: Bước 1 nhận diện tiêu đề `"FormNhapLieu"` khớp quy tắc `excel.exe[FormNhapLieu] = VNI` -> Trả về `2` (VNI) ngay lập tức! Quy tắc của con được ưu tiên, cha không ghi đè con.
  - Khi đóng `FormNhapLieu` quay lại bảng tính `a.xlsx`: Nhận sự kiện tiền cảnh của `a.xlsx` -> Tự động chuyển về `TCVN3`.

### 4.5. Khi nào Fallback về Unicode Mới Được Phép Kích Hoạt?
- **Điều kiện ngặt nghèo**:
  Fallback về Unicode chỉ được phép xảy ra khi thỏa mãn đồng thời 5 điều kiện:
  1. `vAutoSwitchCodeTable == 1` (Tính năng tự động không bị khóa).
  2. `vFallbackToUnicode == 1` (Tùy chọn Fallback được người dùng BẬT).
  3. Cửa sổ con hiện tại KHÔNG khớp bất kỳ quy tắc nào.
  4. Cửa sổ cha/sở hữu KHÔNG khớp bất kỳ quy tắc nào (hoặc không có cửa sổ cha trong cùng PID).
  5. Bảng mã hiện tại chưa phải là Unicode (`vCodeTable != 0`).
- Nếu cửa sổ con hoặc bất kỳ cửa sổ cha nào trong chuỗi sở hữu khớp quy tắc, Fallback **tuyệt đối KHÔNG được kích hoạt**.
- Nếu `vFallbackToUnicode == 0`: Khi cả 2 không khớp, OpenKey giữ nguyên 100% bảng mã hiện tại.

### 4.6. Kiểm soát An toàn: Phòng chống Vòng lặp Vô hạn, Rò rỉ Handle, Cross-Process HWND
- **Cross-process Isolation (Cách ly tiến trình)**:
  - Mọi bước truy vết cha đều lấy `GetWindowThreadProcessId(parentWnd, &parentPid)`.
  - Nếu `parentPid != targetPid` hoặc `targetPid == 0`: Lập tức dừng truy vết! Tuyệt đối không đọc tiêu đề của cửa sổ thuộc tiến trình khác (tránh trường hợp dính cửa sổ Shell desktop, host container của Windows, hay COM server).
- **Phòng chống vòng lặp vô hạn (Infinite Loop Prevention)**:
  - Giới hạn độ sâu duyệt tối đa: `kMaxDepth = 10`.
  - Điều kiện dừng: `parentWnd == NULL || parentWnd == curWnd || !IsWindow(parentWnd) || parentWnd == GetDesktopWindow()`.
- **Rò rỉ Handle (Handle Leak Prevention)**:
  - Các hàm Win32 `GetAncestor`, `GetWindow`, `GetParent`, `GetWindowThreadProcessId`, `IsWindow` là các hàm tra cứu bảng cửa sổ của `USER32.dll`. Chúng **không** tạo handle nhân tử (Kernel Handles), do đó không yêu cầu `CloseHandle` và hoàn toàn không thể gây rò rỉ bộ nhớ hay rò rỉ GDI/User objects.
- **Độ trễ và Tính đáp ứng của Hook Bàn phím**:
  - Tra cứu cấu trúc HWND trong bộ nhớ diễn ra ở mức micro-giây (< 0.05ms).
  - Không đọc ghi đĩa, không can thiệp vào hàm hook phím `keyboardHookProcess`.
  - Luồng gõ phím hoàn toàn trơn tru và mượt mà.

### 4.7. Kiến trúc Single Source of Truth
Mọi thay đổi bảng mã phát sinh từ việc nhận diện cửa sổ (dù là cửa sổ con hay cửa sổ cha) đều phải đi qua luồng chuẩn:
```cpp
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
```
Luồng này đảm bảo:
- `AppDelegate::getInstance()->onTableCode(ruleCode)` cập nhật biến toàn cục `vCodeTable`, lưu registry, kích hoạt cơ chế gõ tắt `onTableCodeChange()` (On-demand Lazy JIT), đồng bộ combobox nếu hộp thoại cài đặt đang mở.
- `SystemTrayHelper::updateData()` cập nhật biểu tượng và dấu tích trong menu khay hệ thống.

---

## 5. Kế hoạch Triển khai Chi tiết cho Agent 2 (Implementation Plan for Developer)

### 5.1. File 1: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.h`
**Thêm 2 khai báo hàm mới**:
```cpp
// Lấy tiêu đề cửa sổ UTF-8 cho một HWND cụ thể
static string getWindowTitleUtf8(HWND hwnd);

// Lấy cửa sổ gốc sở hữu (Root Owner) an toàn trong cùng tiến trình
static HWND getProcessRootOwner(HWND hwnd);
```

### 5.2. File 2: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp`
**Triển khai hàm `getWindowTitleUtf8` và `getProcessRootOwner`, cập nhật `getFrontMostWindowTitleUtf8`**:

```cpp
string OpenKeyHelper::getWindowTitleUtf8(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return "";
    WCHAR titleBuf[1024] = { 0 };
    int len = GetWindowTextW(hwnd, titleBuf, 1024);
    if (len <= 0) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, titleBuf, len, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, titleBuf, len, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

string OpenKeyHelper::getFrontMostWindowTitleUtf8() {
    return getWindowTitleUtf8(GetForegroundWindow());
}

HWND OpenKeyHelper::getProcessRootOwner(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return NULL;

    DWORD targetPid = 0;
    GetWindowThreadProcessId(hwnd, &targetPid);
    if (targetPid == 0) return NULL;

    // Ưu tiên 1: Thử GA_ROOTOWNER duyệt cả chuỗi parent và owner
    HWND hRoot = GetAncestor(hwnd, GA_ROOTOWNER);
    if (hRoot && IsWindow(hRoot) && hRoot != hwnd && hRoot != GetDesktopWindow()) {
        DWORD rootPid = 0;
        GetWindowThreadProcessId(hRoot, &rootPid);
        if (rootPid == targetPid) {
            return hRoot;
        }
    }

    // Ưu tiên 2: Duyệt từng bước qua GW_OWNER / GetParent trong cùng PID (tối đa 10 bước)
    HWND cur = hwnd;
    int depth = 0;
    HWND bestOwner = NULL;
    while (cur && depth++ < 10) {
        HWND next = GetWindow(cur, GW_OWNER);
        if (!next) {
            next = GetParent(cur);
        }
        if (!next || next == cur || !IsWindow(next) || next == GetDesktopWindow()) {
            break;
        }
        DWORD nextPid = 0;
        GetWindowThreadProcessId(next, &nextPid);
        if (nextPid != targetPid) {
            break; // Dừng lại ở ranh giới tiến trình khác
        }
        bestOwner = next;
        cur = next;
    }

    return bestOwner;
}
```

### 5.3. File 3: `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.h`
**Thêm khai báo hàm**:
```cpp
// Lấy bảng mã theo tiêu đề cụ thể (chỉ Pass 1 & Pass 2, không fallback sang quy tắc tiến trình)
static int getCodeTableForTitleOnly(const std::string& exeName, const std::string& windowTitle);

// Phân giải bảng mã cho một cửa sổ (xét ưu tiên: tiêu đề con -> tiêu đề cha/root owner -> quy tắc tiến trình)
static int getCodeTableForWindow(HWND hwnd, const std::string& exeName);
```

### 5.4. File 4: `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp`
**Triển khai hàm `getCodeTableForTitleOnly`, chuẩn hóa `getCodeTableForProcessAndTitle` và triển khai `getCodeTableForWindow`**:

```cpp
int ProcessRuleHelper::getCodeTableForTitleOnly(const std::string& exeName, const std::string& windowTitle) {
    if (!vAutoSwitchCodeTable || windowTitle.empty()) {
        return -1;
    }
    if (!_isInitialized) {
        init();
    }
    std::string lowerExe = toLower(exeName);
    std::string lowerTitle = toLower(windowTitle);

    // Pass 1: So khớp cả tiến trình và mẫu tiêu đề
    for (const auto& item : _ruleItems) {
        if (!item.processName.empty() && !item.titlePattern.empty()) {
            if (lowerExe == item.processName && matchesTitle(lowerTitle, item.titlePattern)) {
                return item.codeTable;
            }
        }
    }

    // Pass 2: So khớp chỉ mẫu tiêu đề chung
    for (const auto& item : _ruleItems) {
        if (item.processName.empty() && !item.titlePattern.empty()) {
            if (matchesTitle(lowerTitle, item.titlePattern)) {
                return item.codeTable;
            }
        }
    }

    return -1;
}

int ProcessRuleHelper::getCodeTableForProcess(const std::string& exeName) {
    if (!vAutoSwitchCodeTable || exeName.empty()) {
        return -1;
    }
    if (!_isInitialized) {
        init();
    }
    std::string lowerExe = toLower(exeName);
    // Pass 3: So khớp chỉ theo tên tiến trình
    for (const auto& item : _ruleItems) {
        if (!item.processName.empty() && item.titlePattern.empty()) {
            if (lowerExe == item.processName) {
                return item.codeTable;
            }
        }
    }
    return -1;
}

int ProcessRuleHelper::getCodeTableForProcessAndTitle(const std::string& exeName, const std::string& windowTitle) {
    int code = getCodeTableForTitleOnly(exeName, windowTitle);
    if (code != -1) return code;
    return getCodeTableForProcess(exeName);
}

int ProcessRuleHelper::getCodeTableForWindow(HWND hwnd, const std::string& exeName) {
    if (!vAutoSwitchCodeTable || !hwnd || !IsWindow(hwnd)) {
        return -1;
    }

    // Bước 1: Kiểm tra tiêu đề của chính cửa sổ hiện tại (Ưu tiên cao nhất)
    std::string childTitle = OpenKeyHelper::getWindowTitleUtf8(hwnd);
    if (!childTitle.empty()) {
        int childCode = getCodeTableForTitleOnly(exeName, childTitle);
        if (childCode != -1) {
            return childCode; // Quy tắc đặc thù của cửa sổ con được kích hoạt
        }
    }

    // Bước 2: Cửa sổ con không có quy tắc riêng -> Truy vết cửa sổ cha/sở hữu cùng PID
    DWORD targetPid = 0;
    GetWindowThreadProcessId(hwnd, &targetPid);
    if (targetPid != 0) {
        // Bước 2a: Kiểm tra cửa sổ sở hữu trực tiếp (Immediate Owner)
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (!hOwner) {
            hOwner = GetParent(hwnd);
        }
        if (hOwner && IsWindow(hOwner) && hOwner != hwnd && hOwner != GetDesktopWindow()) {
            DWORD ownerPid = 0;
            GetWindowThreadProcessId(hOwner, &ownerPid);
            if (ownerPid == targetPid) {
                std::string ownerTitle = OpenKeyHelper::getWindowTitleUtf8(hOwner);
                if (!ownerTitle.empty()) {
                    int ownerCode = getCodeTableForTitleOnly(exeName, ownerTitle);
                    if (ownerCode != -1) {
                        return ownerCode; // Kế thừa từ Immediate Owner
                    }
                }
            }
        }

        // Bước 2b: Kiểm tra cửa sổ gốc sở hữu (Root Owner: GA_ROOTOWNER)
        HWND hRootOwner = OpenKeyHelper::getProcessRootOwner(hwnd);
        if (hRootOwner && IsWindow(hRootOwner) && hRootOwner != hwnd && hRootOwner != hOwner && hRootOwner != GetDesktopWindow()) {
            DWORD rootPid = 0;
            GetWindowThreadProcessId(hRootOwner, &rootPid);
            if (rootPid == targetPid) {
                std::string rootTitle = OpenKeyHelper::getWindowTitleUtf8(hRootOwner);
                if (!rootTitle.empty()) {
                    int rootCode = getCodeTableForTitleOnly(exeName, rootTitle);
                    if (rootCode != -1) {
                        return rootCode; // Kế thừa từ Root Owner
                    }
                }
            }
        }
    }

    // Bước 3: Nếu không có quy tắc tiêu đề nào, kiểm tra quy tắc theo tiến trình (Pass 3)
    return getCodeTableForProcess(exeName);
}
```

### 5.5. File 5: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`
**Cập nhật `winEventProcCallback` (dòng 725-741)**:

```cpp
	// 1. Process & Title recognition rule (only active when not locked)
	if (vAutoSwitchCodeTable) {
		HWND hActiveWnd = GetForegroundWindow();
		if (!hActiveWnd && hwnd) hActiveWnd = hwnd;

		int ruleCode = ProcessRuleHelper::getCodeTableForWindow(hActiveWnd, exe);
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

---

## 6. Ma trận Kiểm thử Toàn diện (Comprehensive Test Matrix)

Dưới đây là ma trận 14 kịch bản kiểm thử bao phủ toàn bộ các trường hợp thông thường và trường hợp biên (edge cases) được bàn giao cho Agent 2 và Agent 3:

| Mã TC | Tên Kịch bản & Mục tiêu | Cấu hình trong `process_rules.ini` | Trạng thái Ban đầu | Hành vi Kích hoạt | Kết quả Kỳ vọng | Điểm Kiểm tra Kỹ thuật (Technical Checkpoint) |
|---|---|---|---|---|---|---|
| **TC-01** | UserForm mở từ file Excel có quy tắc TCVN3 (Ca chuẩn) | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | Đang ở file `a.xlsx` (Bảng mã: TCVN3) | Mở UserForm (tiêu đề `"UserForm1"`) | Bảng mã vẫn giữ nguyên TCVN3, không bị fallback về Unicode | `getCodeTableForWindow` phát hiện `childTitle` không khớp, truy vết `XLMAIN`, khớp `excel.exe[a]`, trả về `1`. |
| **TC-02** | Gõ tiếng Việt trong UserForm | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | Đang trong UserForm (TCVN3) | Gõ từ tắt hoặc văn bản tiếng Việt có dấu (ví dụ font `.VnTime`) | Xuất ra đúng ký tự chuẩn TCVN3 (ABC), không bị vỡ font | Macro JIT và Engine xử lý luồng phím theo `vCodeTable = 1`. |
| **TC-03** | Chuyển đổi qua lại giữa UserForm và Sheet Excel | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | UserForm modeless đang mở | Click qua lại giữa bảng tính và UserForm nhiều lần | Bảng mã duy trì liên tục là TCVN3, không nhấp nháy chuyển mã | Cả 2 cửa sổ đều phân giải ra mã `1`, không gọi cập nhật dư thừa. |
| **TC-04** | UserForm có cấu hình quy tắc riêng | `excel.exe[a] = TCVN3`<br>`excel.exe[FormVNI] = VNI`<br>`fallback_to_unicode = 1` | Đang ở file `a.xlsx` (TCVN3) | Mở UserForm có tiêu đề `"FormVNI"` | Tự động chuyển ngay sang VNI (ưu tiên quy tắc con) | Bước 1 trong `getCodeTableForWindow` khớp tiêu đề con, trả về `2` trước khi kiểm tra cha. |
| **TC-05** | Đóng UserForm có quy tắc riêng quay về Sheet | Cấu hình như TC-04 | Đang ở `"FormVNI"` (VNI) | Đóng UserForm, tiêu điểm về lại sheet `a.xlsx` | Tự động chuyển lại về TCVN3 | `EVENT_SYSTEM_FOREGROUND` kích hoạt cho `a.xlsx`, nhận diện lại TCVN3. |
| **TC-06** | Hộp thoại con có tiêu đề rỗng (`""`) | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | Đang ở file `a.xlsx` (TCVN3) | Mở thanh công cụ popup hoặc dialog không có Caption | Bảng mã giữ nguyên TCVN3 | `childTitle` rỗng, bỏ qua Bước 1, Bước 2/3 truy vết thành công `XLMAIN`. |
| **TC-07** | Hộp thoại modal lồng nhiều cấp (Nested Dialogs) | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | Đang trong UserForm | UserForm bật tiếp `MsgBox "Thông báo"` hoặc Open File | Bảng mã giữ nguyên TCVN3 | Chuỗi truy vết 2 cấp: MsgBox -> UserForm -> `XLMAIN` thành công. |
| **TC-08** | File Excel không có quy tắc khi Fallback BẬT | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | Đang ở `a.xlsx` (TCVN3) | Mở hoặc chuyển sang file `c.xlsx` (không có quy tắc) | Tự động fallback về Unicode (`0`) | Cả con lẫn cha đều không khớp quy tắc; điều kiện fallback thỏa mãn. |
| **TC-09** | File Excel không có quy tắc khi Fallback TẮT | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 0` | Đang ở `a.xlsx` (TCVN3) | Chuyển sang file `c.xlsx` (không có quy tắc) | Giữ nguyên TCVN3 (`1`), không đổi mã | `ruleCode == -1` và `vFallbackToUnicode == 0` -> không đổi bảng mã. |
| **TC-10** | Chuyển từ UserForm sang ứng dụng ngoài (Notepad) khi Fallback BẬT | `excel.exe[a] = TCVN3`<br>`fallback_to_unicode = 1` | Đang trong UserForm (TCVN3) | Nhấn Alt-Tab sang `notepad.exe` | Tự động fallback về Unicode (`0`) | `notepad.exe` khác PID, không có quy tắc, fallback kích hoạt đúng chuẩn. |
| **TC-11** | An toàn ranh giới tiến trình (Cross-process safety) | `excel.exe[a] = TCVN3` | Trong ứng dụng ngoài hoặc Desktop | Cửa sổ có parent thuộc tiến trình khác | Tuyệt đối không duyệt nhầm sang tiến trình lạ | Kiểm tra `ownerPid != targetPid` chặn ngay lập tức. |
| **TC-12** | Khóa tính năng tự động chuyển bảng mã | `enabled = 0` hoặc phím `Ctrl + Shift + F12` | Đang ở Unicode | Mở UserForm trong `a.xlsx` | Bảng mã giữ nguyên Unicode, không tự động chuyển | `if (!vAutoSwitchCodeTable) return -1;` ngắn mạch ngay đầu hàm. |
| **TC-13** | Kiểm tra độ trễ & Stress-test đóng mở liên tục | `excel.exe[a] = TCVN3`<br>`excel.exe[b] = UNICODE` | Mở đồng thời cả `a.xlsx` và `b.xlsx` | Đóng mở UserForm và switch cửa sổ liên tục với tốc độ cao | Độ trễ < 0.05ms, không rò rỉ bộ nhớ, 0 crash, gõ phím mượt mà | Không có I/O đĩa, không có cấp phát bộ nhớ động trong vòng lặp API. |
| **TC-14** | Tương thích Phím tắt thủ công & Menu Khay | Tùy ý | Đang trong UserForm | Nhấn `Ctrl + Shift + F1` hoặc click menu Tray đổi mã | Đổi mã thành công, Macro JIT và Tray đồng bộ tức thời | Tuân thủ tuyệt đối Single Source of Truth `AppDelegate::onTableCode`. |

---

## 7. Đánh giá Tính toàn vẹn & Hướng dẫn Biên dịch (Integrity & Build Guidance)

1. **Nguyên tắc Sửa đổi Tối thiểu (Minimal Change Principle)**:
   - Toàn bộ thay đổi chỉ tập trung vào 3 tệp nguồn Win32: `OpenKeyHelper.h/cpp`, `ProcessRuleHelper.h/cpp`, và `OpenKey.cpp`.
   - Engine cốt lõi (`Engine.cpp`, `Macro.cpp`, `Vietnamese.cpp`) giữ nguyên 100%.
   - Giao diện người dùng và phím tắt giữ nguyên 100%.
2. **Kịch bản Biên dịch**:
   - Chạy `build.bat` tại thư mục gốc `c:\Users\03102025\Desktop\OpenKey`.
   - Kết quả mong đợi: `OpenKey.exe` được biên dịch thành công 0 lỗi.
   - Lưu ý môi trường: Nếu môi trường thiếu `windres.exe` hay `clang++` trong biến môi trường PATH mặc định, cần đảm bảo PATH được bổ sung đường dẫn chứa công cụ biên dịch trước khi gọi lệnh.
3. **Phân giao Tiếp theo**:
   - **Agent 2 (Developer)**: Nhận bản kế hoạch kỹ thuật này và triển khai mã nguồn chính xác theo các phần 5.1 đến 5.5, sau đó chạy kịch bản build.
   - **Agent 3 (Reviewer & QA)**: Phản biện độc lập các trường hợp biên, kiểm tra mã nguồn không có cheat/hardcode, đối chiếu 14 kịch bản kiểm thử trong Ma trận Test, và cập nhật tài liệu kỹ thuật vào `DOCS_AUTO_ENCODING_AND_HOTKEY.md` và `CHANGELOG.md`.
