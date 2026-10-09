# Báo cáo Khảo sát & Kế hoạch Triển khai (Agent 1: Explorer & Planner)

## 1. Observation (Các quan sát thực tế)

### 1.1. Hiện trạng Macro Engine (`Macro.h` & `Macro.cpp`)
- **Tệp**: `Sources/OpenKey/engine/Macro.h`
  - Dòng 19-23: Cấu trúc `MacroData`:
    ```cpp
    struct MacroData {
        string macroText; //ex: "ms"
        string macroContent; //ex: "millisecond"
        vector<Uint32> macroContentCode; //converted of macroContent
    };
    ```
  - Dòng 60-63: Khai báo hàm chuẩn của Engine:
    ```cpp
    /**
     * When table code changed, we have to call this function to reload all macroContentCode
     */
    void onTableCodeChange();
    ```
- **Tệp**: `Sources/OpenKey/engine/Macro.cpp`
  - Dòng 29-65: Hàm `convert(const string& str, vector<Uint32>& outData)` chuyển đổi chuỗi UTF-8 sang mã ký tự theo bảng mã hiện hành (`vCodeTable`):
    ```cpp
    for (map<Uint32, vector<Uint16>>::iterator it = _codeTable[0].begin(); it != _codeTable[0].end(); ++it) {
        ...
        outData.push_back(_codeTable[vCodeTable][it->first][k] | CHAR_CODE_MASK);
        ...
    }
    ```
  - Dòng 242-246: Triển khai hàm `onTableCodeChange()`:
    ```cpp
    void onTableCodeChange() {
        for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
            convert(it->second.macroContent, it->second.macroContentCode);
        }
    }
    ```
  - `macroContent` luôn lưu giữ chuỗi văn bản gốc định dạng UTF-8; chỉ có `macroContentCode` là bộ nhớ đệm (cache) chứa mã phím được dịch theo bảng mã mục tiêu (`vCodeTable`). Khi gọi `onTableCodeChange()`, toàn bộ macro trong bộ nhớ được ánh xạ lại một cách nguyên vẹn từ chuỗi UTF-8 gốc sang bảng mã mới.

### 1.2. Đối chiếu thiết kế gốc trên bản macOS (`AppDelegate.m` & `OpenKey.mm`)
- **Tệp**: `Sources/OpenKey/macOS/ModernKey/AppDelegate.m`
  - Dòng 460-466: Khi đổi bảng mã trên macOS:
    ```objc
    - (void)onCodeTableChanged:(int)index {
        [[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"CodeTable"];
        vCodeTable = index;
        [self fillData];
        [viewController fillData];
        OnTableCodeChange();
    }
    ```
- **Tệp**: `Sources/OpenKey/macOS/ModernKey/OpenKey.mm`
  - Dòng 228-235:
    ```cpp
    void OnTableCodeChange() {
        onTableCodeChange();
        if (vRememberCode) {
            queryFrontMostApp();
            setAppInputMethodStatus(string(_frontMostApp.UTF8String), vLanguage | (vCodeTable << 1));
            saveSmartSwitchKeyData();
        }
    }
    ```
  - Tác giả gốc đã thiết kế rõ ràng: Bất kỳ khi nào `vCodeTable` thay đổi, hàm `onTableCodeChange()` của Engine bắt buộc phải được kích hoạt để nạp lại `macroContentCode`.

### 1.3. Khuyết thiếu trong bản Win32 hiện tại
1. **Trong `AppDelegate.cpp`**:
   - Dòng 300-309: Hàm `AppDelegate::onTableCode(const int & code)`:
     ```cpp
     void AppDelegate::onTableCode(const int & code) {
         APP_SET_DATA(vCodeTable, code);
         if (mainDialog) {
             mainDialog->fillData();
         }
         if (vRememberCode) {
             setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
             saveSmartSwitchKeyData();
         }
     }
     ```
     **Quan sát**: Hoàn toàn **KHÔNG có lệnh gọi `onTableCodeChange()`**!
   - Dòng 174-207: Hàm `AppDelegate::onDefaultConfig()` gán `APP_SET_DATA(vCodeTable, 0);` cũng không kích hoạt `onTableCodeChange()`.

2. **Trong `MainControlDialog.cpp`**:
   - Dòng 392-398: Khi người dùng chọn Combobox bảng mã:
     ```cpp
     else if (hCombobox == comboBoxTableCode) {
         APP_SET_DATA(vCodeTable, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
         if (vRememberCode) {
             setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
             saveSmartSwitchKeyData();
         }
     }
     ```
     **Quan sát**: Combobox trên hộp thoại chính tự thao tác trực tiếp với Registry và `SmartSwitchKey`, bỏ qua hoàn toàn `AppDelegate::onTableCode()` và không gọi `onTableCodeChange()`.

3. **Trong `OpenKey.cpp`**:
   - Dòng 533-552: Phím tắt nhanh `Ctrl + Shift + F1` gọi `AppDelegate::getInstance()->onTableCode(0)`, và `Ctrl + Shift + F2` gọi `AppDelegate::getInstance()->onTableCode(1)`.
   - Dòng 726-740: Callback `winEventProcCallback` (tự động nhận diện ứng dụng/tiêu đề và fallback Unicode) gọi `AppDelegate::getInstance()->onTableCode(ruleCode)` và `AppDelegate::getInstance()->onTableCode(0)`.
   - Cả hai luồng này đều tập trung về `AppDelegate::onTableCode(...)`. Do `AppDelegate::onTableCode` không có `onTableCodeChange()`, dữ liệu macro không hề được cập nhật.

4. **Trong `SystemTrayHelper.cpp`**:
   - Dòng 147-161: Menu chuột phải System Tray cho 5 bảng mã (`POPUP_UNICODE`, `POPUP_TCVN3`, `POPUP_VNI_WINDOWS`, `POPUP_UNICODE_COMPOUND`, `POPUP_VN_LOCALE_1258`) đều gọi `AppDelegate::getInstance()->onTableCode(...)`.

### 1.4. Môi trường biên dịch hiện hành
- Đã chạy thử nghiệm lệnh `cmd /c build.bat`.
- Kết quả: Mã nguồn biên dịch qua `clang++` và `windres` với UTF-8 codepage 65001 thành công tốt đẹp (Exit Code 0), tạo ra `OpenKey.exe` hợp lệ.

---

## 2. Logic Chain (Chuỗi lập luận & Phân tích Nguyên nhân gốc rễ)

1. Từ **Mục 1.1**: Engine gõ tiếng Việt của OpenKey lưu trữ nội dung macro dưới dạng văn bản gốc UTF-8 (`macroContent`), và duy trì mảng `macroContentCode` để xuất phím tự động. Hàm `convert()` sử dụng trực tiếp biến toàn cục `vCodeTable` để tra bảng mã `_codeTable[vCodeTable]`.
2. Khi ứng dụng khởi động (`OpenKeyInit()` trong `OpenKey.cpp`), `initMacroMap()` chuyển đổi các macro theo giá trị `vCodeTable` ban đầu (thường là Unicode = 0).
3. Khi người dùng chuyển bảng mã trong quá trình sử dụng:
   - Qua phím tắt `Ctrl + Shift + F1/F2`: gọi `AppDelegate::onTableCode`.
   - Qua menu khay hệ thống (System Tray): gọi `AppDelegate::onTableCode`.
   - Qua tự động nhận diện tiến trình (`ProcessRuleHelper`) hoặc Fallback về Unicode: gọi `AppDelegate::onTableCode`.
   - Qua Combobox trong hộp thoại Cài đặt (`MainControlDialog`): trực tiếp cập nhật `vCodeTable`.
4. Từ **Mục 1.2** và **1.3**: Trên macOS, mỗi khi `vCodeTable` thay đổi thì `onTableCodeChange()` được gọi ngay lập tức. Nhưng trên Win32, không có bất kỳ điểm nào trong mã nguồn gọi `onTableCodeChange()`.
5. Hệ quả: Biến `vCodeTable` đã chuyển sang giá trị mới (ví dụ 1: TCVN3 hoặc 2: VNI), nhưng `macroContentCode` trong bộ nhớ vẫn giữ nguyên các byte mã của bảng mã cũ. Khi người dùng gõ từ viết tắt, `handleMacro()` xuất ra các byte của bảng mã cũ, dẫn đến lỗi vỡ font hoặc sai ký tự tiếng Việt.
6. **Giải pháp kiến trúc chuẩn**:
   - Đặt lệnh gọi `onTableCodeChange()` vào tâm điểm điều phối `AppDelegate::onTableCode(const int & code)` trong `AppDelegate.cpp`.
   - Chuẩn hóa sự kiện chọn combobox `comboBoxTableCode` trong `MainControlDialog.cpp` để ủy quyền hoàn toàn cho `AppDelegate::getInstance()->onTableCode(code)`, loại bỏ trùng lặp code và đảm bảo luồng xử lý duy nhất (Single Source of Truth).
   - Đảm bảo `AppDelegate::onDefaultConfig()` cũng gọi `onTableCodeChange()`.
   - Bổ sung `SystemTrayHelper::updateData()` vào trong `AppDelegate::onTableCode()` để mọi kênh chuyển đổi bảng mã đều đồng bộ trạng thái checkmark trên System Tray một cách nhất quán.

---

## 3. Caveats (Các giả định & Lưu ý hiệu năng)

1. **Hiệu năng và độ trễ (Zero-lag)**:
   - Hàm `onTableCodeChange()` chỉ duyệt qua map bộ nhớ `macroMap` (thường từ vài chục đến vài trăm phần tử) và thao tác chuỗi trong RAM. Thời gian thực thi là cấp micro-giây (< 0.1ms).
   - Hàm này chỉ được kích hoạt khi có sự kiện chuyển đổi bảng mã (người dùng bấm phím tắt, click menu, hoặc chuyển cửa sổ ứng dụng có quy tắc khác bảng mã hiện tại).
   - Trong quá trình gõ phím thông thường (`keyboardHookProcess`), `onTableCodeChange()` hoàn toàn không chạy, do đó đảm bảo 100% không ảnh hưởng đến tốc độ gõ phím.
2. **Quy tắc SmartSwitchKey vs ProcessRuleHelper**:
   - Theo thiết kế đã thống nhất trong tài liệu kỹ thuật trước (`DOCS_AUTO_ENCODING_AND_HOTKEY.md`), việc chuyển bảng mã theo cửa sổ do `ProcessRuleHelper` và tùy chọn `vFallbackToUnicode` đảm nhiệm. Logic `SmartSwitchKey` ở nhánh foreground chỉ đảo ngôn ngữ Anh/Việt (`_languageTemp & 0x01`). Điều này cần được giữ nguyên để tránh xung đột bảng mã không mong muốn.
3. **Thêm/Sửa Macro trong MacroDialog**:
   - Trong `MacroDialog.cpp`, khi người dùng bấm nút Thêm/Sửa macro, hàm `addMacro()` trong `Macro.cpp` được gọi. Hàm `addMacro()` đã gọi `convert(macroContent, data.macroContentCode)` theo `vCodeTable` hiện hành tại thời điểm đó. Khi người dùng đổi bảng mã tiếp theo, `onTableCodeChange()` sẽ tiếp tục đồng bộ lại macro mới này. Hoàn toàn tương thích và an toàn.

---

## 4. Conclusion & Kế hoạch Chi tiết cho Agent 2 (Dev)

### 4.1. Danh sách tệp cần chỉnh sửa
1. `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.cpp`
2. `Sources/OpenKey/win32/OpenKey/OpenKey/MainControlDialog.cpp`

### 4.2. Chi tiết mã nguồn cần thay đổi

#### Tệp 1: `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.cpp`

**Vị trí 1: Hàm `AppDelegate::onTableCode(const int & code)` (khoảng dòng 300)**
- *Hiện tại:*
```cpp
void AppDelegate::onTableCode(const int & code) {
	APP_SET_DATA(vCodeTable, code);
	if (mainDialog) {
		mainDialog->fillData();
	}
	if (vRememberCode) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
}
```
- *Đổi thành:*
```cpp
void AppDelegate::onTableCode(const int & code) {
	APP_SET_DATA(vCodeTable, code);
	onTableCodeChange();
	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
	if (vRememberCode) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
}
```

**Vị trí 2: Hàm `AppDelegate::onDefaultConfig()` (khoảng dòng 178)**
- *Hiện tại:*
```cpp
	APP_SET_DATA(vCodeTable, 0);
```
- *Đổi thành:*
```cpp
	APP_SET_DATA(vCodeTable, 0);
	onTableCodeChange();
```

*(Ghi chú: Header `Macro.h` đã được include gián tiếp thông qua `stdafx.h` $\rightarrow$ `Engine.h` $\rightarrow$ `Macro.h`, hàm `onTableCodeChange()` sẵn sàng được gọi mà không cần include thêm).*

---

#### Tệp 2: `Sources/OpenKey/win32/OpenKey/OpenKey/MainControlDialog.cpp`

**Vị trí: Hàm `MainControlDialog::onComboBoxSelected` (khoảng dòng 388-400)**
- *Hiện tại:*
```cpp
void MainControlDialog::onComboBoxSelected(const HWND& hCombobox, const int& comboboxId) {
    if (hCombobox == comboBoxInputType) {
        APP_SET_DATA(vInputType, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
    }
    else if (hCombobox == comboBoxTableCode) {
        APP_SET_DATA(vCodeTable, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
        if (vRememberCode) {
            setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
            saveSmartSwitchKeyData();
        }
    }
    SystemTrayHelper::updateData();
}
```
- *Đổi thành:*
```cpp
void MainControlDialog::onComboBoxSelected(const HWND& hCombobox, const int& comboboxId) {
    if (hCombobox == comboBoxInputType) {
        APP_SET_DATA(vInputType, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
        SystemTrayHelper::updateData();
    }
    else if (hCombobox == comboBoxTableCode) {
        int code = (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0);
        AppDelegate::getInstance()->onTableCode(code);
    }
}
```
*(Ghi chú: Khi gọi `AppDelegate::getInstance()->onTableCode(code)`, nó sẽ tự động cập nhật Registry, gọi `onTableCodeChange()`, cập nhật `mainDialog->fillData()`, cập nhật `SystemTrayHelper::updateData()`, và lưu nhớ bảng mã cho ứng dụng. Luồng xử lý hoàn toàn chuẩn hóa).*

---

## 5. Verification Method (Phương pháp Kiểm thử & Đánh giá cho Agent 3)

### 5.1. Biên dịch
1. Mở cửa sổ dòng lệnh tại thư mục gốc: `C:\Users\Administrator\Desktop\OpenKey`.
2. Thực thi lệnh biên dịch:
   ```cmd
   cmd /c build.bat
   ```
3. Kiểm tra kết quả:
   - Mã thoát (Exit Code) phải là `0`.
   - Tệp thực thi `OpenKey.exe` được cập nhật tại thư mục gốc và thư mục `Sources\OpenKey\win32\OpenKey\OpenKey\OpenKey.exe`.

### 5.2. Kịch bản Kiểm thử Tính đúng đắn của Bảng mã Gõ tắt (Macro)
Chuẩn bị trước một macro mẫu:
- Từ tắt: `ms`
- Nội dung: `Cộng hòa Xã hội Chủ nghĩa Việt Nam`

| STT | Kịch bản kiểm thử | Thao tác | Kết quả kỳ vọng |
|---|---|---|---|
| **TC-01** | Bảng mã mặc định Unicode | Khởi động OpenKey ở bảng mã Unicode. Mở Notepad. Gõ `ms ` (kèm khoảng trắng). | Xuất ra đúng chuỗi: `Cộng hòa Xã hội Chủ nghĩa Việt Nam` định dạng Unicode. |
| **TC-02** | Chuyển sang TCVN3 bằng Hotkey | Nhấn tổ hợp phím `Ctrl + Shift + F2`. Quan sát thông báo "Bảng mã: TCVN3 (ABC)". Mở phần mềm/font TCVN3 (ví dụ font `.VnTime`). Gõ `ms `. | Xuất ra đúng các ký tự thuộc bảng mã TCVN3 (ABC). |
| **TC-03** | Chuyển sang Unicode bằng Hotkey | Nhấn tổ hợp phím `Ctrl + Shift + F1`. Quan sát thông báo "Bảng mã: Unicode". Mở Notepad. Gõ `ms `. | Xuất ra chuỗi Unicode chuẩn xác, không còn dính mã TCVN3. |
| **TC-04** | Chuyển sang VNI qua System Tray | Nhấp chuột phải vào biểu tượng OpenKey ở khay hệ thống $\rightarrow$ Chọn **VNI Windows**. Mở ứng dụng font VNI (ví dụ `VNI-Times`). Gõ `ms `. | Xuất ra đúng các ký tự mã VNI Windows. |
| **TC-05** | Chuyển bảng mã qua Hộp thoại Chính | Mở Bảng điều khiển OpenKey $\rightarrow$ Chọn **Unicode Tổ hợp** trong Combobox Bảng mã. Gõ `ms `. | Xuất ra chuỗi ký tự Unicode tổ hợp. |
| **TC-06** | Tự động chuyển bảng mã theo tiến trình / Excel | Cấu hình trong `process_rules.ini` quy tắc `excel.exe[a] = TCVN3`. Mở file Excel tên `a.xlsx`. Gõ `ms ` trong Excel. | Tự động đổi sang TCVN3 và gõ tắt ra ký tự TCVN3. |
| **TC-07** | Fallback về Unicode khi rời ứng dụng | Bật tùy chọn "Fallback về Unicode khi rời ứng dụng". Rời Excel sang Notepad. Gõ `ms `. | Tự động fallback về Unicode và gõ tắt ra ký tự Unicode. |
| **TC-08** | Thêm mới Macro khi đang ở bảng mã TCVN3 | Trong khi đang ở TCVN3, mở hộp thoại Gõ tắt $\rightarrow$ Thêm macro mới `dc` = `Độc lập Tự do Hạnh phúc`. Gõ `dc ` $\rightarrow$ kiểm tra ra TCVN3. Chuyển sang Unicode $\rightarrow$ gõ `dc ` $\rightarrow$ kiểm tra ra Unicode. | Macro mới thêm tự động thích ứng với cả 2 bảng mã. |
| **TC-09** | Chuyển đổi liên tục nhiều lần | Nhấn đổi qua lại giữa `Ctrl + Shift + F1` và `Ctrl + Shift + F2` liên tục 10 lần. Sau đó gõ thử `ms `. | Macro luôn xuất ra đúng theo bảng mã hiện thời cuối cùng, không crash, không lag. |

### 5.3. Điều kiện Phủ nhận (Invalidation Conditions)
Nếu sau khi chỉnh sửa xảy ra bất kỳ điều nào sau đây, bản vá bị coi là không đạt:
- Gõ tắt `ms` ở TCVN3 nhưng vẫn ra ký tự Unicode (hoặc ngược lại).
- Xuất hiện hiện tượng giật/lag khi gõ phím hoặc chuyển cửa sổ.
- Biến đổi trạng thái checkmark trên System Tray bị lệch so với bảng mã thực tế.
- Lỗi biên dịch trong `build.bat`.
