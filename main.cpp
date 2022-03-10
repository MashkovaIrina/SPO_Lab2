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
    hslot = CreateMailslot(
                           lpName,//имя почтового ящика
                           NULL, //максимальный размер сообщений не установлен
                           MAILSLOT_WAIT_FOREVER,//при операциях нет таймаута
                           &Attributes//атрибуты доступа вызванные ранее с помощью функции create_security_attributes()
                           );
    if (hslot == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
                printf("Ошибка при создании почтового ящика.Код ошибки: %d\n", GetLastError());
                return FALSE;
    }

    process = TRUE;
    return TRUE;
}

// 2.1.Запрос на получение информации о почтовом ящике

BOOL GetInfo(HANDLE hSlot, DWORD* cbMessage, DWORD* cMessage)
{
    DWORD cbMessageL, cMessageL;
    BOOL infoMail;

    infoMail = GetMailslotInfo(hSlot,//дескриптор почтового ящика
                           (LPDWORD) NULL,//нет ограничение на размер сообщения
                           &cbMessageL,//размер следующего сообщения
                           &cMessageL,//кол-во сообщений в ящике
                           (LPDWORD) NULL//без таймаута чтения
                           );

    if (infoMail)
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
BOOL WriteMessage(HANDLE hFile, LPCTSTR lpszMessage)
{
    BOOL hwrite;
    DWORD cbWritten;

    hwrite = WriteFile(hFile,//дескриптор почтового ящика
                       lpszMessage,//указатаель на буфер, содержащий сообщение
                       (lstrlen(lpszMessage) + 1) * sizeof(TCHAR),//кол-во записываемых байтов
                       &cbWritten,//переменная, получающая кол-во байтов
                       (LPOVERLAPPED)NULL//указатель не нужен поэтму NULL
                       );

    if (!hwrite)
    {
        printf("Сообщение отправлено\n");
        return TRUE;
    }
    else
    {
        printf("Ошибка записи сообщения.Код ошибки: %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}


//2.3.Запрос на чтения сообщения из почтового ящика

BOOL ReadMessage(HANDLE hFile)
{
    DWORD lpNextSize, lpMessageCount, lpNumberOfBytesRead;
    BOOL fresult;

    lpNextSize = lpMessageCount = lpNumberOfBytesRead = 0;

    fresult = GetMailslotInfo(hFile,NULL,&lpNextSize,&lpMessageCount,NULL);

    if (!fresult)
    {
        printf("Ошибка при получении информации о почтовом ящике.Код ошибки: %d\n", GetLastError());
        return FALSE;
    }

    if (lpNextSize == MAILSLOT_NO_MESSAGE)
    {
        printf("Почтовый ящик пуст\n");
        return TRUE;
    }


    string Message(lpNextSize,'\0');
    DWORD nNumberOfBytesToRead = sizeof(Message);

    fresult = ReadFile(hFile,//дескриптор почтового ящика
                     &Message[0],//буфер сообщений
                      nNumberOfBytesToRead,//число байтов для чтения
                      &lpNumberOfBytesRead,//число прочитанных байтов
                      NULL//указатель не нужен поэтму NULL
                      );

    if (fresult != 0)
    {
        cout << Message;
    return TRUE;
    }
    printf("Ошибка при попытке прочитать содержимое файла.Код ошибки: %d\n", GetLastError());
        return FALSE;
}

// Главная программа и отключение от почтового ящика с помощью функции CloseHandle()

int main()
{
    DWORD* cbMessage;
    DWORD* cMessage;
    cbMessage = cMessage = 0;
    string Name_mailslot;

    setlocale(LC_ALL, "Russian");

    cout << "Введите почтовый ящик ";
    cin >> Name_mailslot;

    LPCTSTR SLOT_NAME = Name_mailslot.c_str();

    HANDLE hSlot;
    int user;
    bool process;

    bool fresult=MakeSlot(SLOT_NAME, hSlot, process);
    if (!fresult)
        return 0;

    printf("Процесс запущен\n");
    printf("Выберете процесс:\n [1] - Сервер\n [2] - Клиент\n");
    cin>>user;
    if(user==1)
        printf("Сервер\n Список команд:\n[1] - Получить информацию о количестве сообщений\n [2] - Чтение полученных сообщений\n [4] - Завершение работы программы\n");
    if(user==2)
        printf("Клиент\n Список команд:\n[1] - Получить информацию о количестве сообщений\n [3] - Отправка сообщений\n [4] - Завершение работы программы\n");
    if((user!=1)&&(user!=2))
        printf("Команда введена некорректно.\n");
    string command;
    while (1)
    {
        cout << ">";
        cin >> command;
        if (command == "1")
        {
            GetInfo(hSlot, cbMessage, cMessage);
        }
        else if (command == "4")
        {
            CloseHandle(hSlot);
            return 0;
        }
        else if ((command == "2") && (user==1))
        {
            ReadMessage(hSlot);
        }
        else if (command == "3")
        {
            printf("Введите текст сообщения. Ввод продолжиться пока строка не пустая.\n");

            string message, str;

            getline(cin, str);
             do
            {
                getline(cin, str);
                message += str + '\n';
            } while (!str.empty());

            WriteMessage(hSlot, (LPCTSTR)message.c_str());
        }
        else if ((command != "1") || (command != "2") || (command != "3") || (command == "4"))
        {
            printf("Команда введена некорректно.\n");
        }
    }
    return 0;
}
