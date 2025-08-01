#include <QtCore>
#include <catch.hpp>
// Assigned in the first few lines of main.
inline QString term_path()
{
    // File written out by cmake. First line is the path of pepp terminal executable.
    auto dir = QCoreApplication::applicationDirPath();;
    QDir d(dir);
    for (auto filename : d.entryList()) {
        auto file = QFile(filename);
        if (filename.contains("pepp-term") && QFileInfo(file).isExecutable())
            return d.absoluteFilePath(filename);
    }
    throw std::logic_error("Couldn't find terminal app");
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
