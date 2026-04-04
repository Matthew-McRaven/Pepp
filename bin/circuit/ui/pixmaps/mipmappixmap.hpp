#pragma once
#include <QPainter>
#include <QSvgRenderer>
#include "prerotatedpixmap.hpp"

// Mipmap generation should stop as soon as either constraint is violated.
struct MipmapConstraint {
  // Stop generation if either x or y is <= this value.
  int min_dimension = 4;
  // Stop generation once this many levels beyond 0 have been generated..
  int max_levels = 5;
};

// Contains prerotated pixmaps for each mip level, with level 0 being full resolution.
// Various name constructors create mip levels and rotations automaticall from both SVGs and pixmaps.
// Utility methods (best_for) automatically select the correct mip level for a given destination size.
// Size is alays reduced by a power-of-2 when downsampling.
class MipmappedPrerotatedPixmap {
public:
  // Conveience methods to rasterize from SVG and size.
  static MipmappedPrerotatedPixmap from_left(QSvgRenderer &svg, QSize base_size, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from_right(QSvgRenderer &svg, QSize base_size, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from_up(QSvgRenderer &svg, QSize base_size, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from_down(QSvgRenderer &svg, QSize base_size, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from(QSvgRenderer &svg, QSize base_size, Direction dir,
                                        MipmapConstraint constraints = {});
  // Conveience methods to downsample from a raster. Downsampling will be much lossier than SVG.
  static MipmappedPrerotatedPixmap from_left(const QPixmap &pixmap, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from_right(const QPixmap &pixmap, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from_up(const QPixmap &pixmap, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from_down(const QPixmap &pixmap, MipmapConstraint constraints = {});
  static MipmappedPrerotatedPixmap from(const QPixmap &pixmap, Direction dir, MipmapConstraint constraints = {});

  // Return the best prerotated pixmap to render at dst_size.
  const PrerotatedPixmap &best_for(QSize dst_size) const;
  // Convenience overloads for getting one of the individual prerotated pixmaps directly.
  const QPixmap &best_for(QSize dst_size, Direction dir) const;
  const QPixmap &best_for(QSize dst_size, int angle) const;
  // Access a specific mip level. Level 0 is full resolution.
  // Out-of-range requests clamp to the smallest available mip.
  const PrerotatedPixmap &at_level(int level) const;
  // Pick the best mip for drawing at dst_size, given the level-0 source size.
  // Returns the level where source >= destination (i.e. we downsample, not upsample).
  int level_for(QSize dst_size) const;
  // Number of available mips levels (including 0).
  int level_count() const;

private:
  std::vector<PrerotatedPixmap> _levels;
  using FromFn = PrerotatedPixmap (*)(const QPixmap &);
  static MipmappedPrerotatedPixmap build_raster(const QPixmap &src, FromFn from_fn, MipmapConstraint cn);
  static QPixmap render_svg(QSvgRenderer &svg, QSize size);
  static MipmappedPrerotatedPixmap build_svg(QSvgRenderer &svg, QSize base_size, FromFn from_fn, MipmapConstraint cn);
};
