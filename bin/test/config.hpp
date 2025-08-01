#include <QtCore>
#include <catch.hpp>
inline QString term_path()
{
  auto dir = QCoreApplication::applicationDirPath();
  QDir d(dir);
#ifdef Q_OS_WIN32
  static const QString filename = "pepp-term.exe";
#else
  static const QString filename = "pepp-term";
#endif
  if (d.exists(filename)) return d.absoluteFilePath(filename);
  throw std::logic_error("Couldn't find terminal app, tried: " + d.absoluteFilePath(filename).toStdString());
}

inline void wait_return(QProcess &proc, auto exitCode = 0)
{
    REQUIRE(proc.waitForStarted());
    REQUIRE(proc.waitForFinished());
    auto actualCode = proc.exitCode();
    if (actualCode != exitCode) {
        qDebug() << proc.readAllStandardError();
        auto fmt = QStringLiteral("Process exited with unexpected exit status %1").arg(actualCode);
        FAIL(fmt.toStdString().c_str());
    }
}

// Replace platform-newlines with \n
inline QString out(QProcess &proc)
{
    auto out = proc.readAllStandardOutput();
    out.replace("\r", "");
    return out;
}
