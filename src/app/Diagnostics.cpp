#include "app/Diagnostics.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QRegularExpression>

#include <windows.h>

#include <dbghelp.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace avloop::diagnostics {

namespace {

constexpr qint64 kMaxLogBytes = 10LL << 20;
constexpr int kKeepDays = 7;

QMutex logMutex;
QString logDir;
QString logPath;
QFile logOut;
QtMessageHandler previous = nullptr;
wchar_t dumpDir[MAX_PATH] = {};

// The day's file, or its next part once a part has grown past 10 MB.
QString pathFor(const QDate& day) {
    const QString base =
        QDir(logDir).filePath(QStringLiteral("avloop-%1").arg(day.toString(QStringLiteral("yyyyMMdd"))));
    for (int part = 1;; ++part) {
        const QString p = part == 1 ? base + QStringLiteral(".log") : QStringLiteral("%1-%2.log").arg(base).arg(part);
        if (!QFileInfo::exists(p) || QFileInfo(p).size() < kMaxLogBytes) return p;
    }
}

void write(const QString& line) {
    const QMutexLocker lock(&logMutex);
    const QString want = pathFor(QDate::currentDate());
    if (want != logPath || !logOut.isOpen() || logOut.size() >= kMaxLogBytes) {
        logOut.close();
        logPath = pathFor(QDate::currentDate());
        logOut.setFileName(logPath);
        if (!logOut.open(QIODevice::Append | QIODevice::Text)) return;
    }
    logOut.write(line.toUtf8());
    logOut.flush();
}

void handler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString module = QStringLiteral("app");
    QString text = message;
    if (context.category && std::strcmp(context.category, "default") != 0) {
        module = QString::fromLatin1(context.category);
    } else {
        static const QRegularExpression tag(QStringLiteral("^\\[([A-Za-z0-9_-]+)\\]\\s*"));
        if (const QRegularExpressionMatch m = tag.match(text); m.hasMatch()) {
            module = m.captured(1);
            text = text.mid(m.capturedLength());
        }
    }
    const char* level = type == QtWarningMsg    ? "warning: "
                        : type == QtCriticalMsg ? "error: "
                        : type == QtFatalMsg    ? "fatal: "
                                                : "";
    write(QStringLiteral("%1 [%2] %3%4\n")
              .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")), module, QLatin1String(level), text));
    if (previous) previous(type, context, message);
}

void removeOld(const QString& dir) {
    const QDateTime limit = QDateTime::currentDateTime().addDays(-kKeepDays);
    for (const QFileInfo& f :
         QDir(dir).entryInfoList({QStringLiteral("avloop-*.log"), QStringLiteral("avloop-*.dmp")}, QDir::Files))
        if (f.lastModified() < limit) QFile::remove(f.absoluteFilePath());
}

LONG WINAPI writeDump(EXCEPTION_POINTERS* info) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t path[MAX_PATH];
    swprintf_s(path, L"%s\\avloop-%04d%02d%02d-%02d%02d%02d.dmp", dumpDir, t.wYear, t.wMonth, t.wDay, t.wHour,
               t.wMinute, t.wSecond);
    const HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION exception{GetCurrentThreadId(), info, FALSE};
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                          MINIDUMP_TYPE(MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory), &exception,
                          nullptr, nullptr);
        CloseHandle(file);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

void installLog(const QString& dir) {
    logDir = dir;
    QDir().mkpath(dir);
    removeOld(dir);
    previous = qInstallMessageHandler(handler);
}

void closeLog() {
    qInstallMessageHandler(previous);
    const QMutexLocker lock(&logMutex);
    logOut.close();
}

QString logFile() {
    const QMutexLocker lock(&logMutex);
    return logPath;
}

void installCrashHandler(const QString& dir) {
    QDir().mkpath(dir);
    const std::wstring native = QDir::toNativeSeparators(dir).toStdWString();
    wcsncpy_s(dumpDir, native.c_str(), _TRUNCATE);
    SetUnhandledExceptionFilter(writeDump);
}

} // namespace avloop::diagnostics
