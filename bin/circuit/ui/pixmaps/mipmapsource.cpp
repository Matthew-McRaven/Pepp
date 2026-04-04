#include "mipmapsource.hpp"
#include <QDebug>
#include <QFile>

MipmapSource MipmapSource::from_svg_file(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "MipmapSource::from_svg_file: failed to open" << path << ":" << file.errorString();
    return MipmapSource{};
  }
  return MipmapSource(file.readAll());
}

MipmapSource MipmapSource::from_svg_data(QByteArray data) { return MipmapSource(data); }

MipmapSource MipmapSource::from_pixmap(QPixmap pixmap) { return MipmapSource(std::move(pixmap)); }

bool MipmapSource::is_svg() const noexcept { return std::holds_alternative<QByteArray>(_source); }

bool MipmapSource::is_raster() const noexcept { return std::holds_alternative<QPixmap>(_source); }

bool MipmapSource::is_valid() const noexcept { return !std::holds_alternative<std::monostate>(_source); }

MipmappedPrerotatedPixmap MipmapSource::build(QSize base_size, Direction dir, MipmapConstraint constraints) const {
  if (auto *bytes = std::get_if<QByteArray>(&_source)) {
    QSvgRenderer renderer(*bytes);
    renderer.setAspectRatioMode(Qt::KeepAspectRatio);
    return MipmappedPrerotatedPixmap::from(renderer, base_size, dir, constraints);
  }
  return MipmappedPrerotatedPixmap::from(std::get<QPixmap>(_source), dir, constraints);
}

MipmapSource::MipmapSource(QByteArray data) : _source(std::move(data)) {}

MipmapSource::MipmapSource(QPixmap data) : _source(std::move(data)) {}
