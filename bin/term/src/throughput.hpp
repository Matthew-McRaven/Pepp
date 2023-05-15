#include "./task.hpp"
#include <QtCore>

class ThroughputTask : public Task {
  Q_OBJECT
public:
  ThroughputTask(QObject *parent = nullptr);
  ;
  ~ThroughputTask() = default;
  void run();
};
