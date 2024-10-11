#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>

class RDPClient {
public:
    DWORD SecurityLayer;
    DWORD UserAuthentication;
    std::wstring Server;

    RDPClient(const std::wstring& server) : Server(server) {}

    void Connect() {
        // Connect to the RDP server
        std::wcout << "Connecting to " << Server << "...\n";
        // Your connection logic here
    }

    void HandleDisconnection(int discReason) {
        // Handle disconnection reason
        std::wstring errMsg = L"Unknown error code: 0x" + std::to_wstring(discReason);
        switch (discReason) {
            case 1:
                errMsg = L"Local disconnection.";
                break;
            case 2:
                errMsg = L"Disconnected by user.";
                break;
            // Handle other cases...
            default:
                break;
        }
        MessageBox(NULL, (LPCSTR)errMsg.c_str(), "Disconnected", MB_OK | MB_ICONERROR);

        // Restore registry settings
        RestoreRegistrySettings();

        exit(0); // Terminate the application
    }

private:
    void RestoreRegistrySettings() {
        HKEY hKey;
        LONG lResult;

        // Open the key for RDP settings
        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", 0, KEY_SET_VALUE, &hKey);
        if (lResult != ERROR_SUCCESS) {
            // Handle error
            return;
        }

        // Write back the original settings
        RegSetValueEx(hKey, "SecurityLayer", 0, REG_DWORD, (BYTE*)&SecurityLayer, sizeof(DWORD));
        RegSetValueEx(hKey, "UserAuthentication", 0, REG_DWORD, (BYTE*)&UserAuthentication, sizeof(DWORD));

        // Close the key
        RegCloseKey(hKey);
    }
};

int main() {
    // Create RDP client
    RDPClient rdp(L"127.0.0.1");

    // FormCreate equivalent

    // Set registry values
    HKEY hKey;
    LONG lResult;

    // Open the key for RDP settings
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        // Handle error
        return 1;
    }

    // Read original values
    DWORD dwSize = sizeof(DWORD);
    RegQueryValueEx(hKey, "SecurityLayer", NULL, NULL, (BYTE*)&rdp.SecurityLayer, &dwSize);
    RegQueryValueEx(hKey, "UserAuthentication", NULL, NULL, (BYTE*)&rdp.UserAuthentication, &dwSize);

    // Modify registry values
    rdp.SecurityLayer = 0;
    rdp.UserAuthentication = 0;
    RegSetValueEx(hKey, "SecurityLayer", 0, REG_DWORD, (BYTE*)&rdp.SecurityLayer, sizeof(DWORD));
    RegSetValueEx(hKey, "UserAuthentication", 0, REG_DWORD, (BYTE*)&rdp.UserAuthentication, sizeof(DWORD));

    // Close the key
    RegCloseKey(hKey);

    // Simulate disconnection reason
    int disconnectionReason = 2; // Sample reason

    // RDPDisconnected equivalent
    rdp.HandleDisconnection(disconnectionReason);

    return 0;
}