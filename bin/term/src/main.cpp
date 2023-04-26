#include <CLI11.hpp>
#include <QDebug>
#include <QtCore>

class Term : public QObject {
  Q_OBJECT
public:
  Term(int argc, char **argv, QObject *parent = nullptr)
      : QObject(parent), argc(argc), argv(argv) {}

public slots:
  void run() {
    CLI::App app{"Magic app", "pepp"};
    app.set_help_flag("-h,--help", "test");
    // auto help = app.add_flag("-h,--help");
    app.add_flag("-f", "test");

    try {
      app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
      std::cout << app.help();
    } catch (const CLI::ParseError &e) {
      qDebug() << e.what();
      emit finished(1);
      return;
    }
    qDebug() << "Here";
    emit finished(0);
  }

signals:
  void finished(int);

private:
  int argc;
  char **argv;
};

#include "main.moc"

int main(int argc, char **argv) {
  QCoreApplication a(argc, argv);
  Term *task = new Term(argc, argv, &a);
  QObject::connect(task, &Term::finished, &a, QCoreApplication::exit);
  QTimer::singleShot(0, task, &Term::run);
  return a.exec();
}
