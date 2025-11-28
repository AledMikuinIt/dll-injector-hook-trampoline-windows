#include <Windows.h>
#include <iostream>


int main() {

    STARTUPINFOW si = { sizeof(si) }; 
    PROCESS_INFORMATION pi;      

    BOOL succes = CreateProcessW(
        L"main.exe",    
        nullptr,      
        nullptr,      
        nullptr,      
        FALSE,        
        CREATE_SUSPENDED,   
        nullptr,    
        nullptr,        
        &si,         
        &pi            
    );

    if(!succes) {
        std::cerr << "Erreur CreateProcces\n";  
        return 1;                              
    }
    std::cout << "Process créé en suspendu !"; 



    // Allocation de la mémoire dans le process cible
    const wchar_t* dllPath = L"PATH_TO_DLL"; 

    LPVOID remoteMemory = VirtualAllocEx(  
        pi.hProcess,
        nullptr,
        (wcslen(dllPath) + 1) * sizeof(wchar_t),    
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if(!remoteMemory) {
        std::cerr << "Erreur VirtualAllocEx";
        return 1;
    }

    // Ecriture de la DLL
    SIZE_T bytesWritten;       
    BOOL wpm = WriteProcessMemory(      
        pi.hProcess,
        remoteMemory,
        dllPath,
        (wcslen(dllPath) + 1) * sizeof(wchar_t),       
        &bytesWritten                                 
    );

    if (!wpm) {
        std::cerr << "Erreur WriteProcessMemory";
        return 1;
    }

    // Créer thread distant pour charger la dll
    HANDLE hThread = CreateRemoteThread( 
        pi.hProcess,        
        nullptr,            
        0,                 
        (LPTHREAD_START_ROUTINE)LoadLibraryW,
        remoteMemory,                      
        0,                              
        nullptr                         
    );

    if(!hThread) {
        std::cerr << "Erreur CreateRemoteThread";
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE); 
    CloseHandle(hThread); 

    ResumeThread(pi.hThread);
    std::cout << "DLL injectée, process repris";

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;
}