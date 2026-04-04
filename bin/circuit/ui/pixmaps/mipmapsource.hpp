#pragma once

#include <variant>
#include "mipmappixmap.hpp"

// Helper class which retains the source data for our mipmap.
// We want to retain this information in the MipmapStore to allow us to rebuild mipmaps if display settings change.
class MipmapSource {
public:
  static MipmapSource from_svg_file(const QString &path);
  static MipmapSource from_svg_data(QByteArray data);
  static MipmapSource from_pixmap(QPixmap pixmap);

  bool is_svg() const noexcept;
  bool is_raster() const noexcept;
  bool is_valid() const noexcept;

  MipmappedPrerotatedPixmap build(QSize base_size, Direction dir, MipmapConstraint constraints = {}) const;
  MipmapSource() = default;

private:
  MipmapSource(QByteArray);
  MipmapSource(QPixmap);

  std::variant<std::monostate, QByteArray, QPixmap> _source = std::monostate{};
};
