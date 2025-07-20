## MemoryWriter

MemoryWriter is a simple C++ clone of an existing software called Cheat Engine.  
It allows you to scan and modify memory values of running processes using a Qt-based GUI.

---

### 🎥 Demo https://www.youtube.com/watch?v=AR4-xXdryYE&ab_channel=Xeku

[![MemoryWriter Demo](https://img.youtube.com/vi/AR4-xXdryYE/0.jpg)](https://www.youtube.com/watch?v=AR4-xXdryYE&ab_channel=Xeku)

---

### ✅ Requirements

- Windows only (for now)  
- Qt 6.9.1 (MinGW 64-bit)

---

### ⚙️ How to Build It Yourself

> A prebuilt release is available [here](https://github.com/Xekuuu/MemoryWriter/releases)

1. Open the project in **Qt Creator**
2. Switch to **Release** build mode
3. Build the project
4. Run the following command to deploy dependencies:  
   *(adjust paths as needed)*

```bash
C:\qt\6.9.1\mingw_64\bin\windeployqt.exe D:\MemoryWriter\build\Desktop_Qt_6_9_1_MinGW_64_bit-Release\MemoryWriter.exe
