#include <Windows.h>
#include <string>

void* trampoline = nullptr;
const SIZE_T patchSize = 7;

void Hook(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    MessageBoxW(
        NULL,
        L"Hooked !",
        L"TRAMPOLINE",
        MB_OK
    );


    auto origFunc = (decltype(&MessageBoxA))trampoline;
    origFunc(hWnd, lpText, lpCaption, uType);
}

BOOL APIENTRY DllMain(
    HMODULE hmodule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {


        // Module where MessageBoxA is located
        HMODULE hmod = GetModuleHandleA("user32.dll");
        if(!hmod) return FALSE;

        // Get original Address from the MessageBoxA function in user32.dll
        BYTE* originalAddress = (BYTE*)GetProcAddress(hmod, "MessageBoxA");
        if(!originalAddress) return FALSE;

        // Create the trampoline
        trampoline = VirtualAlloc(
            nullptr,
            patchSize + 12,         // + 12 here for an ABSOLUTE JMP because 10 for the mov rax imm64 and 2 for jmp rax
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );
        if(!trampoline) return FALSE;

        // Copy the instruction in the trampoline
        memcpy(
            trampoline,
            originalAddress,
            patchSize
        );


        // Create Continue Address
        BYTE* continueAddress = (BYTE*)originalAddress + patchSize;

        // Create End Trampoline Address
        BYTE* trampolineAddress = (BYTE*)trampoline + patchSize;

        // Create the ABSOLUTE JMP
        // mov rax, imm64 = 0x48 B8 imm64
        trampolineAddress[0] = 0x48;
        trampolineAddress[1] = 0xB8;
        memcpy(
            trampolineAddress + 2,
            &continueAddress,
            8       // 8 bytes here because its 64 bits address for imm64
        );
        // jmp rax = 0xFF E0
        trampolineAddress[10] = 0xFF;
        trampolineAddress[11] = 0xE0;


        // Protection for the original Address
        DWORD oldProtect;
        VirtualProtect(
            originalAddress,
            patchSize,
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        );

        intptr_t originalOffset = (intptr_t)Hook - (intptr_t(originalAddress + 5));

        // Force JMP
        originalAddress[0] = 0xE9; // Opcode for jmp instruction

        // copy the next bytes for the hook address
        memcpy(
            originalAddress + 1,
            &originalOffset,
            4
        );

        if(patchSize > 5)
        memset(originalAddress + 5, 0x90, patchSize - 5);

        // Put back Protection
        VirtualProtect(
            originalAddress,
            patchSize,
            oldProtect,
            &oldProtect
        );
        break;
        }
    default:
        break;
    }
    return TRUE;

}