#include "ConPTY.h"

HRESULT InitializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX* pStartupInfo, HPCON hPC)
{
    HRESULT hr{ E_UNEXPECTED };

    if (pStartupInfo)
    {
        size_t attrListSize{};

        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEX);

        // Get the size of the thread attribute list.
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

        // Allocate a thread attribute list of the correct size
        pStartupInfo->lpAttributeList =
            reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(malloc(attrListSize));

        // Initialize thread attribute list
        if (pStartupInfo->lpAttributeList
            && InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1, 0, &attrListSize))
        {
            // Set Pseudo Console attribute
            hr = UpdateProcThreadAttribute(
                pStartupInfo->lpAttributeList,
                0,
                PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                hPC,
                sizeof(HPCON),
                NULL,
                NULL)
                ? S_OK
                : HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

HRESULT SpawnProcessinConsole(wchar_t* szCommand, HPCON hPC, PROCESS_INFORMATION* piClient)
{
    /* Initialize the necessary startup info struct */
    STARTUPINFOEX startupInfo{};
    HRESULT hr = { E_UNEXPECTED };
    if (S_OK == InitializeStartupInfoAttachedToPseudoConsole(&startupInfo, hPC))
    {
        // Launch ping to emit some text back via the pipe
        hr = CreateProcessW(
            NULL,                           // No module name - use Command Line
            szCommand,                      // Command Line
            NULL,                           // Process handle not inheritable
            NULL,                           // Thread handle not inheritable
            FALSE,                          // Inherit handles
            EXTENDED_STARTUPINFO_PRESENT,   // Creation flags
            NULL,                           // Use parent's environment block
            NULL,                           // Use parent's starting directory 
            &startupInfo.StartupInfo,       // Pointer to STARTUPINFO
            piClient)                      // Pointer to PROCESS_INFORMATION
            ? S_OK
            : GetLastError();
    }
    return hr; 

}
HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, SHORT consoleWidth, SHORT consoleHeight)
{
    HRESULT hr{ E_UNEXPECTED };
    HANDLE hPipePTYIn{ INVALID_HANDLE_VALUE };
    HANDLE hPipePTYOut{ INVALID_HANDLE_VALUE };

    // Create the pipes to which the ConPTY will connect
    if (CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) &&
        CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0))
    {
        // Determine required size of Pseudo Console
        COORD consoleSize{};
        consoleSize.X = consoleWidth;
        consoleSize.Y = consoleHeight;

        // Create the Pseudo Console of the required size, attached to the PTY-end of the pipes
        hr = CreatePseudoConsole(consoleSize, hPipePTYIn, hPipePTYOut, 0, phPC);

        // Note: We can close the handles to the PTY-end of the pipes here
        // because the handles are dup'ed into the ConHost and will be released
        // when the ConPTY is destroyed.
        if (INVALID_HANDLE_VALUE != hPipePTYOut) CloseHandle(hPipePTYOut);
        if (INVALID_HANDLE_VALUE != hPipePTYIn) CloseHandle(hPipePTYIn);
    }

    return hr;
}