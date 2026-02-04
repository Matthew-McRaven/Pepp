#include "renderer.hpp"
#include <QPainter>

CursedCanvas::CursedCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {}

void CursedCanvas::paint(QPainter *painter) {
  QBrush brush(QColor("#007430"));
  painter->setBrush(brush);
  painter->drawRect(0, 0, size().width(), size().height());
}
