# DLL Injector & Inline Hook with Trampoline (Windows)
## Overview

This project demonstrates a `Windows DLL` injection combined with a 64‑bit inline hook using a trampoline.

Unlike a simple hook, this version:
- preserves the original bytes of MessageBoxA
- executes a custom hook function
- then returns to the original API using an absolute JMP trampoline (x64‑safe)

This is the correct method for implementing a stable hook on x64, where 5‑byte relative jumps cannot address arbitrary 64‑bit locations.

## Hook Logic Summary

1. Allocate a trampoline (VirtualAlloc) 
2. Copy the original overwritten instructions (7 bytes) into it
3. Append an absolute 64‑bit jump:
```asm
48 B8 <imm64>     ; mov rax, <address>
FF E0             ; jmp rax
```
4. Patch the first bytes of MessageBoxA with:
```asm
E9 <rel32>        ; jmp Hook
90 90 ...         ; padding if patchSize > 5
```
5. In the hook, call the original function through the trampoline.

## Project Structure
| File           | Description                          |
| -------------- | ------------------------------------ |
| `main.cpp`     | Program calling `MessageBoxA`.       |
| `hook.cpp`     | DLL providing the hook & trampoline. |
| `injector.cpp` | Suspended‑process DLL injector.      |

###  Hook Implementation (Relevant Extract)
```c++
void Hook(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    MessageBoxW(NULL, L"Hooked !", L"TRAMPOLINE", MB_OK);

    // Call original MessageBoxA through trampoline
    auto origFunc = (decltype(&MessageBoxA))trampoline;
    origFunc(hWnd, lpText, lpCaption, uType);
}
```
### Trampoline construction
```c++
trampoline = VirtualAlloc(nullptr, patchSize + 12, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

memcpy(trampoline, originalAddress, patchSize);
BYTE* continueAddress = originalAddress + patchSize;
BYTE* t = (BYTE*)trampoline + patchSize;

t[0]  = 0x48; t[1] = 0xB8;               // mov rax, imm64
memcpy(t + 2, &continueAddress, 8);     // imm64 = address after patch
t[10] = 0xFF; t[11] = 0xE0;             // jmp rax
```
### Overwriting MessageBoxA
```c++
originalAddress[0] = 0xE9;  // JMP rel32
memcpy(originalAddress + 1, &originalOffset, 4);
memset(originalAddress + 5, 0x90, patchSize - 5);
```

## Compilation
1. Build main executable
```
g++ -o main.exe main.cpp -m64
```

2. Build the DLL
```
g++ -shared -o hook.dll hook.cpp -m64
```

3. Build the injector
```
g++ -o injector.exe injector.cpp -m64
```

4. Run
```
.\injector.exe
```


Expected behavior:

- Your custom hook displays "Hooked!".
- Control returns to original MessageBoxA.
- The normal message box appears.

# ⚠️ Notes & Limitations

- Inline hooks require knowing the size of the overwritten instructions (patchSize).
- On x64, absolute jumps require MOV RAX + JMP RAX, as relative jumps cannot always reach 64‑bit addresses.
- DLL injection and hooking can trigger antivirus alarms.
- Use strictly in controlled environments.
