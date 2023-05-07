#pragma once
#include <QtCore>
class Task : public QObject {
  Q_OBJECT
public:
  Task(QObject *parent = nullptr);
  virtual ~Task() = default;
  virtual void run() = 0;
signals:
  void finished(int ret);
};

class ListTask : public Task {
public:
  ListTask(int ed, QObject *parent = nullptr);
  void run() override;

private:
  int ed;
};
