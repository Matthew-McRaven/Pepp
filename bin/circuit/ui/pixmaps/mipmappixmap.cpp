#include "mipmappixmap.hpp"

#include <QDebug>
#include <QRandomGenerator>

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_left(QSvgRenderer &svg, QSize base_size,
                                                               MipmapConstraint constraints) {
  return build_svg(svg, base_size, &PrerotatedPixmap::from_left, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_right(QSvgRenderer &svg, QSize base_size,
                                                                MipmapConstraint constraints) {
  return build_svg(svg, base_size, &PrerotatedPixmap::from_right, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_up(QSvgRenderer &svg, QSize base_size,
                                                             MipmapConstraint constraints) {
  return build_svg(svg, base_size, &PrerotatedPixmap::from_up, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_down(QSvgRenderer &svg, QSize base_size,
                                                               MipmapConstraint constraints) {
  return build_svg(svg, base_size, &PrerotatedPixmap::from_down, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from(QSvgRenderer &svg, QSize base_size, Direction dir,
                                                          MipmapConstraint constraints) {
  switch (dir) {
  case Direction::Left: return from_left(svg, base_size, constraints);
  case Direction::Right: return from_right(svg, base_size, constraints);
  case Direction::Up: return from_up(svg, base_size, constraints);
  case Direction::Down: return from_down(svg, base_size, constraints);
  }
  Q_UNREACHABLE();
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_left(const QPixmap &pixmap, MipmapConstraint constraints) {
  return build_raster(pixmap, &PrerotatedPixmap::from_left, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_right(const QPixmap &pixmap, MipmapConstraint constraints) {
  return build_raster(pixmap, &PrerotatedPixmap::from_right, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_up(const QPixmap &pixmap, MipmapConstraint constraints) {
  return build_raster(pixmap, &PrerotatedPixmap::from_up, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from_down(const QPixmap &pixmap, MipmapConstraint constraints) {
  return build_raster(pixmap, &PrerotatedPixmap::from_down, constraints);
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::from(const QPixmap &pixmap, Direction dir,
                                                          MipmapConstraint constraints) {
  switch (dir) {
  case Direction::Left: return from_left(pixmap, constraints);
  case Direction::Right: return from_right(pixmap, constraints);
  case Direction::Up: return from_up(pixmap, constraints);
  case Direction::Down: return from_down(pixmap, constraints);
  }
  Q_UNREACHABLE();
}

const PrerotatedPixmap &MipmappedPrerotatedPixmap::best_for(QSize dst_size) const {
  return at_level(level_for(dst_size));
}

const QPixmap &MipmappedPrerotatedPixmap::best_for(QSize dst_size, Direction dir) const {
  const auto rotated = best_for(dst_size);
  return rotated.get(dir);
}

const QPixmap &MipmappedPrerotatedPixmap::best_for(QSize dst_size, int angle) const {
  const auto rotated = best_for(dst_size);
  return rotated.get(angle);
}

const PrerotatedPixmap &MipmappedPrerotatedPixmap::at_level(int level) const {
  if (level < 0) level = 0;
  if (level >= int(_levels.size())) level = int(_levels.size()) - 1;
  return _levels[level];
}

int MipmappedPrerotatedPixmap::level_for(QSize dst_size) const {
  if (_levels.empty() || dst_size.isEmpty()) return 0;
  const QSize src = _levels[0].left().size(); // any orientation works for ratio
  const qreal ratio_w = qreal(src.width()) / dst_size.width();
  const qreal ratio_h = qreal(src.height()) / dst_size.height();
  const qreal ratio = qMin(ratio_w, ratio_h); // conservative: pick finer mip
  if (ratio <= 1.0) return 0; // If any ratio<0, then we are upscaling. Pick 0, which is the best available.
  const int level = int(std::floor(std::log2(ratio)));
  const int selected = qBound(0, level, int(_levels.size()) - 1);
  return selected;
}

int MipmappedPrerotatedPixmap::level_count() const { return int(_levels.size()); }

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::build_raster(const QPixmap &src, FromFn from_fn,
                                                                  MipmapConstraint cn) {
  MipmappedPrerotatedPixmap result;
  QPixmap current = src;
  while (current.width() >= cn.min_dimension && current.height() >= cn.min_dimension &&
         result._levels.size() <= cn.max_levels) {
    result._levels.push_back(from_fn(current));
    current = QPixmap(
        current.scaled(current.width() / 2, current.height() / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
  return result;
}

QPixmap MipmappedPrerotatedPixmap::render_svg(QSvgRenderer &svg, QSize size) {
  QPixmap pm(size);
  pm.fill(Qt::transparent);
  Q_ASSERT(pm.size() == size);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  svg.render(&p, QRectF(QPointF(0, 0), QSizeF(size)));
  return pm;
}

MipmappedPrerotatedPixmap MipmappedPrerotatedPixmap::build_svg(QSvgRenderer &svg, QSize base_size, FromFn from_fn,
                                                               MipmapConstraint cn) {
  MipmappedPrerotatedPixmap result;
  QSize size = base_size;
  // const QString prefix = QString::number(QRandomGenerator::global()->generate(), 16);

  while (size.width() >= cn.min_dimension && size.height() >= cn.min_dimension &&
         result._levels.size() <= cn.max_levels) {
    const auto rendered = render_svg(svg, size);
    /*const QString basename =
        QString("%1_level%2_%3x%4").arg(prefix).arg(result._levels.size()).arg(size.width()).arg(size.height());
    if (!rendered.save(basename + ".png", "PNG"))  qWarning() << "Failed to save mip dump:" << basename + ".png";*/
    const auto prerotated = from_fn(rendered);
    /*prerotated.left().save(basename + "_left.png", "PNG");
    prerotated.right().save(basename + "_right.png", "PNG");
    prerotated.up().save(basename + "_up.png", "PNG");
    prerotated.down().save(basename + "_down.png", "PNG");*/
    result._levels.push_back(prerotated);
    size = QSize(size.width() / 2, size.height() / 2);
  }
  return result;
}
