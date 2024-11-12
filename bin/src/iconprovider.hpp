#pragma once

#include <QColorTransform>
#include <QDomDocument>
#include <QFile>
#include <QGraphicsColorizeEffect>
#include <QGraphicsPixmapItem>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QQuickImageProvider>
#include <QRegularExpression>
#include <QSvgRenderer>

// Helper class
class PreferenceAwareImageProvider : public QQuickImageProvider {
public:
  PreferenceAwareImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

  QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override {
    // If the image exists, return it without modification. Allows of passthrough of non vector graphics.
    if (auto path = ":/icons" + id; QFile::exists(path))
      return QImage(path);
    // Otherwise, we assume that the image has tags like _disabled or _enabled.
    // The current algorithm to generate those tagged images from a base image only work for SVG images.
    else if (!id.endsWith("svg"))
      return {};
    // If the icon path contains relevant tags, strip the tag(s) and apply modifications to the SVG's fill.
    // That suffix is an indicator that we should render the SVG with a disabled color from the current theme.
    static const auto disabled = QRegularExpression(R"(_disabled)");
    static const auto dark = QRegularExpression(R"(_dark)");
    static const auto re = QRegularExpression(R"(_((dis|en)abled|light|dark))");
    bool isDisabled = id.contains(disabled);
    bool isDark = id.contains(dark);
    QString iconPath = ":/icons/" + QString(id).replace(re, "");

    QFile f(iconPath);
    QByteArray contents;
    try {
      f.open(QIODevice::ReadOnly | QIODevice::Text);
      contents = f.readAll();
      f.close();
    } catch ([[maybe_unused]] QFile::FileError &e) {
      if (f.isOpen())
        f.close();
      return {};
    }

    if (isDisabled || isDark) {
      QDomDocument doc;
      if (!doc.setContent(contents))
        return {};
      // TODO: pick fill color from current palette.
      static const QStringList colors = {/*00*/ "#000000", /*01*/ "#CCCCCC", /*10*/ "#FFFFFF",
                                         /*11*/ "#CCCCCC"};
      if (QDomElement svgElement = doc.documentElement(); svgElement.tagName() == "svg")
        svgElement.setAttribute("fill", colors[(isDark ? 2 : 0) | (isDisabled ? 1 : 0)]);
      contents = doc.toByteArray();
    }
    // Render the SVG's XML to a QImage.
    QXmlStreamReader reader(contents);
    QSvgRenderer renderer;
    renderer.load(&reader);
    if (renderer.isValid()) {
      // Per documentation, must respect requested size if it is valid.
      auto image = QImage(requestedSize.isValid() ? requestedSize : renderer.defaultSize(), QImage::Format_ARGB32);
      if (image.isNull())
        return {};
      if (size)
        *size = image.size();
      image.fill(Qt::transparent);
      QPainter painter(&image);
      if (!painter.isActive())
        return {};
      renderer.render(&painter);
      return image;
    } else
      return {};
  }
};
