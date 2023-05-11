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
