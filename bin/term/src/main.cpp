#include <QDebug>
#include <QtCore>

class Term : public QObject {
  Q_OBJECT
public:
  Term(QObject *parent = 0) : QObject(parent) {}

public slots:
  void run() {
    qDebug() << "Here";
    emit finished();
  }

signals:
  void finished();
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);
  Term *task = new Term(&a);
  QObject::connect(task, &Term::finished, &a, &QCoreApplication::quit);
  QTimer::singleShot(0, task, &Term::run);
  return a.exec();
}
