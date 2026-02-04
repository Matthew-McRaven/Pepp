#pragma once
#include <QQuickPaintedItem>
class CursedCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCanvas)

public:
  CursedCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;
};
