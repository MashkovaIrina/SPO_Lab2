#define WINVER 0x0502
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sddl.h>

using namespace std;

//Параметр lpSecurityAttributes для разрешения доступа всем процессам

static PSECURITY_DESCRIPTOR create_security_descriptor()
{
   const char* sddl ="D:(A;OICI;GRGW;;;AU)(A;OICI;GA;;;BA)";
   PSECURITY_DESCRIPTOR security_descriptor = NULL;
   ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, SDDL_REVISION_1, &security_descriptor, NULL);
   return security_descriptor;
}

static SECURITY_ATTRIBUTES create_security_attributes()
{
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = create_security_descriptor();
    attributes.bInheritHandle = FALSE;
    return attributes;
}

// 1.Запрос имени почтового ящика

BOOL WINAPI MakeSlot(LPCTSTR lpName, HANDLE& hslot, bool& process)
{
    auto Attributes = create_security_attributes();
    hslot = CreateMailslot(lpName,NULL,MAILSLOT_WAIT_FOREVER,&Attributes);

    if (hslot == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();

        if (error == ERROR_INVALID_NAME||error == ERROR_ALREADY_EXISTS)
        {
            process = FALSE;
            hslot = CreateFile(lpName,GENERIC_WRITE,FILE_SHARE_READ,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE)NULL);

            if (hslot == INVALID_HANDLE_VALUE)
            {
                DWORD error = GetLastError();
                printf("Ошибка при выполнении функции CreateFile().Код ошибки: %d\n", GetLastError());
                return FALSE;
            }
        }
        else
        {
            printf("Ошибка при выполнении функции CreateFile().Код ошибки: %d\n", GetLastError());
            return FALSE;
        }
    }
    else
        process = TRUE;
    return TRUE;
}

// 2.1.Запрос на получение информации о почтовом ящике

BOOL GetInfo(HANDLE hSlot, DWORD* cbMessage, DWORD* cMessage)
{
    DWORD cbMessageL, cMessageL;
    BOOL info;

    info = GetMailslotInfo(hSlot,(LPDWORD) NULL,&cbMessageL,&cMessageL,(LPDWORD) NULL);

    if (info)
    {
        printf("Найдено %i сообщений на почтовом ящике\n", cMessageL);
        if (cbMessage != NULL)
            *cbMessage = cbMessageL;
        if (cMessage != NULL)
            *cMessage = cMessageL;
        return TRUE;
    }
    else
    {
        printf("Ошибка при выполнении функции GetMailslotInfo().Код ошибки: %d\n", GetLastError());
        return FALSE;
    }
}
// 2.2.Зпрос на помещение сообщения в почтовый ящик
BOOL Write(HANDLE hFile, LPCTSTR lpszMessage)
{
    BOOL hwrite;
    DWORD cbWritten;

    hwrite = WriteFile(hFile,lpszMessage,(lstrlen(lpszMessage) + 1) * sizeof(TCHAR),&cbWritten,(LPOVERLAPPED)NULL);

    if (hwrite!=0)
    {
        printf("Сообщение отправлено\n");
        return TRUE;
    }
    else
    {
        printf("Ошибка при выполнении функции WriteFile().Код ошибки: %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

//2.3.Запрос на получения сообщения из почтового ящика

BOOL Read(HANDLE hFile)
{
    DWORD lpNextSize, lpMessageCount, lpNumberOfBytesRead;
    BOOL infoR;

    lpNextSize = lpMessageCount = lpNumberOfBytesRead = 0;

    infoR = GetMailslotInfo(hFile,NULL,&lpNextSize,&lpMessageCount,NULL);

    if (lpNextSize == MAILSLOT_NO_MESSAGE)
    {
        printf("Почтовый ящик пуст\n");
        return TRUE;
    }

    if (infoR==0)
    {
        printf("Ошибка при выполнении функции GetMailslotInfo.Код ошибки: %d\n", GetLastError());
        return FALSE;
    }

    string Message(lpNextSize,'\0');
    DWORD nNumberOfBytesToRead = sizeof(Message);

    infoR = ReadFile(hFile,&Message[0], nNumberOfBytesToRead,&lpNumberOfBytesRead,NULL);

    if (infoR == 0)
    {
        printf("Ошибка при попытке прочитать содержимое файла.Код ошибки: %d\n", GetLastError());
        return FALSE;
    }
    cout << Message;
    return TRUE;
}

// Главная программа и отключение от почтового ящика с помощью функции CloseHandle()

int main()
{
    DWORD* cbMessage;
    DWORD* cMessage;
    cbMessage = cMessage = 0;
    string Name_mailslot;

    setlocale(LC_ALL, "Russian");

    cout << "Введите имя почтового ящика ";
    cin >> Name_mailslot;

    LPCTSTR SLOT_NAME = Name_mailslot.c_str();

    HANDLE hSlot;
    bool process;

    bool result=MakeSlot(SLOT_NAME, hSlot, process);
    if (result==FALSE)
        return 0;

    printf("Процесс запущен");
    printf((process) ? "Сервер\n" : "Клиент\n");
    printf("Список команд:\n[check] - Получить информацию о количестве сообщений\n");
    printf((process) ? "[read] - Чтение полученных сообщений\n" : "[write] - Отправка сообщений\n");
    printf("[quit] - Завершение работы программы\n");

    string command;
    while (1)
    {
        cout << ">";
        cin >> command;
        if (command == "check")
        {
            GetInfo(hSlot, cbMessage, cMessage);
        }
        else if (command == "quit")
        {
            CloseHandle(hSlot);
            return 0;
        }
        else if ((command == "read") && (process))
        {
            Read(hSlot);
        }
        else if ((command == "write") && (!process))
        {
            printf("Введите текст сообщения. Ввод продолжиться пока строка не пустая.\n");

            string message, str;

            getline(cin, str);
             do
            {
                getline(cin, str);
                message += str + '\n';
            } while (!str.empty());

            Write(hSlot, (LPCTSTR)message.c_str());
        }
        else if ((command != "write") || (command != "check") || (command != "read") || (command == "quit"))
        {
            printf("Команда введена некорректно.Введите одну из предложенных команд:\n[check]- информация о количестве сообщений\n");
            printf((process) ? "[read]- чтение полученных сообщений\n" : "[write]- отправка сообщений\n");
            printf("[quit]- завершение работы программы\n");
        }
    }
    return 0;
}
