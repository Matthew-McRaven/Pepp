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
    // We depend on properties of the SVG renderer, so exclude other icon types.
    if (!id.endsWith("svg"))
      return {};
    // If the icon path ends in disabled, we need to strip the suffix, since no file exists with "_disabled.svg'
    // That suffix is an indicator that we should render the SVG with a disabled color from the current theme.
    static const auto re = QRegularExpression(R"(_disabled)");
    bool isDisabled = id.contains(re);
    QString iconPath = ":/icons/" + QString(id).replace(re, "");

    QFile f(iconPath);
    QByteArray contents;
    try {
      f.open(QIODevice::ReadOnly | QIODevice::Text);
      contents = f.readAll();
      f.close();
    } catch (QFile::FileError &err) {
      if (f.isOpen())
        f.close();
      return {};
    }

    // If a disabled icon is requested, parse the document and change the "fill" attribute
    if (isDisabled) {
      QDomDocument doc;
      if (!doc.setContent(contents))
        return {};
      // TODO: pick fill color from current palette.
      if (QDomElement svgElement = doc.documentElement(); svgElement.tagName() == "svg")
        svgElement.setAttribute("fill", "#CCCCCC");
      contents = doc.toByteArray();
    }
    // Render the SVG's XML to a QImage.
    QXmlStreamReader reader(contents);
    QSvgRenderer renderer;
    renderer.load(&reader);
    if (renderer.isValid()) {
      // Per documentation, must respect requested size if it is valid.
      auto image = QImage(requestedSize.isValid() ? requestedSize : renderer.defaultSize(), QImage::Format_ARGB32);
      image.fill(Qt::transparent);
      QPainter painter(&image);
      renderer.render(&painter);

      if (size)
        *size = image.size();
      return image;
    } else
      return {};
  }
};
