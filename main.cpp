#include "mainwindow.h"

#include <windows.h>
#include <shellapi.h>
#include <QCoreApplication>
#include <QApplication>
#include <QNetworkProxy>
#include <QProcess>

//#ifdef Q_OS_WIN
// static bool isRunningAsAdmin() {
//     BOOL isAdmin = FALSE;
//     PSID adminGroup = nullptr;
//     SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
//     if (::AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
//                                    0,0,0,0,0,0, &adminGroup)) {
//         ::CheckTokenMembership(nullptr, adminGroup, &isAdmin);
//         ::FreeSid(adminGroup);
//     }
//     return isAdmin == TRUE;
// }

// static bool relaunchElevated(int argc, char* argv[]) {
//       // 1) 可靠获取 EXE 路径（不要在 QApplication 创建前用 applicationFilePath）
//   wchar_t exePath[MAX_PATH];
//   DWORD n = ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
//   if (n == 0 || n >= MAX_PATH) return false;

//   // 2) 拼参数（简单拼接；如需更严谨可逐个加引号）
//   QString params;
//   for (int i=1; i<argc; ++i) {
//     QString a = QString::fromLocal8Bit(argv[i]);
//     if (a.contains(' ')) a = "\"" + a + "\"";
//     params += a + " ";
//   }
//   std::wstring wparams = params.toStdWString();

//   SHELLEXECUTEINFOW sei{};
//   sei.cbSize = sizeof(sei);
//   sei.fMask = SEE_MASK_NOCLOSEPROCESS;
//   sei.lpVerb = L"runas";
//   sei.lpFile = exePath;                                   // 必须是 exe 的绝对路径
//   sei.lpParameters = wparams.c_str();                     // 仅参数
//   // 可选：设置工作目录为 EXE 目录，防止相对路径异常
//   wchar_t workDir[MAX_PATH]; lstrcpynW(workDir, exePath, MAX_PATH);
//   for (int i = lstrlenW(workDir)-1; i>=0; --i) { if (workDir[i]==L'\\') { workDir[i]=0; break; } }
//   sei.lpDirectory = workDir;
//   sei.nShow = SW_SHOWNORMAL;

//   if (!ShellExecuteExW(&sei)) return false;
//   return true;
// }
//#endif

int main(int argc, char *argv[])
{
// #ifdef Q_OS_WIN
//   if (!isRunningAsAdmin()) {
//     if (relaunchElevated(argc, argv)) {
//       return 0;  // 让提权后的实例继续运行
//     }
//     // 提权失败就继续以普通权限运行，或给出提示后 return
//   }
// #endif
    QApplication a(argc, argv);
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    MainWindow w;
    w.show();
    return a.exec();
}
