#pragma once

#include <QString>

namespace avloop::diagnostics {

// The log (spec §11.3): `<dir>\avloop-YYYYMMDD.log`, each line `HH:MM:SS.mmm [module] text` (the module: the message's
// category, a leading "[media]" of its text, else "app"); a file over 10 MB goes on in `-2`, `-3` …; logs and dumps
// older than 7 days are deleted at start. Qt's messages still reach stderr as well.
void installLog(const QString& dir);
QString logFile(); // the file being written
// Qt's own handler back and the file closed: before main returns (a lab run's folder can then be removed, and no
// message of the teardown reaches a file already gone).
void closeLog();

// An unhandled exception writes `<dir>\avloop-YYYYMMDD-HHMMSS.dmp` (MiniDumpWriteDump) before the process ends.
void installCrashHandler(const QString& dir);

} // namespace avloop::diagnostics
