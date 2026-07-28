#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle> //  Fusion style

#include <lunasvg.h>

using namespace lunasvg;

static const char kLandspaceContent[] = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 600" width="800" height="600">
  <!-- Background (Sky) -->
  <rect width="800" height="600" class="sky"/>

  <!-- Sun -->
  <circle cx="650" cy="150" r="80" class="sun" />

  <!-- Clouds -->
  <ellipse cx="200" cy="150" rx="100" ry="40" class="cloud" />
  <ellipse cx="250" cy="200" rx="120" ry="50" class="cloud" />
  <ellipse cx="500" cy="80" rx="150" ry="60" class="cloud" />
  <ellipse cx="550" cy="120" rx="120" ry="50" class="cloud" />

  <!-- Mountains -->
  <polygon points="0,450 200,200 400,450" class="mountain" />
  <polygon points="200,450 400,100 600,450" class="mountain" />
  <polygon points="400,450 600,250 800,450" class="mountain" />

  <!-- Foreground (Ground) -->
  <rect y="450" width="800" height="150" class="ground" />
</svg>
)SVG";

static const char kSummerStyle[] = R"CSS(
.sky { fill: #4A90E2; }
.sun { fill: #FF7F00; }
.mountain { fill: #2E3A59; }
.cloud { fill: #FFFFFF; opacity: 0.8; }
.ground { fill: #2E8B57; }
)CSS";

static const char kWinterStyle[] = R"CSS(
.sky { fill: #87CEEB; }
.sun { fill: #ADD8E6; }
.mountain { fill: #2F4F4F; }
.cloud { fill: #FFFFFF; opacity: 0.8; }
.ground { fill: #FFFAFA; }
)CSS";

int main(int argc, char *argv[])
{
  QGuiApplication app(argc, argv);

  QQuickStyle::setStyle("Fusion");

  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);
  engine.loadFromModule("CircuitDesign", "Main");
  app.setWindowIcon(QIcon{":/icon"});

  qDebug() << "Opening LunaSVG version:" << lunasvg_version_string();
  // See if we can render without crashing on multiple platforms.
  auto document = Document::loadFromData(kLandspaceContent);
  qDebug() << "document width:" << document->width() << "height:" << document->height();

  document->applyStyleSheet(kSummerStyle);
  auto base = QCoreApplication::applicationDirPath();
  {
    auto bitmap = document->renderToBitmap();
    // LunaSVG bitmap is premultiplied ARGB32 (BGRA byte order on little-endian).
    // Convert to a format Qt handles cleanly.
    bitmap.convertToRGBA(); // ensures RGBA byte order

    QImage image(bitmap.data(), bitmap.width(), bitmap.height(), bitmap.stride(),
                 QImage::Format_RGBA8888_Premultiplied);
    qDebug() << "bitmap width:" << bitmap.width() << "height:" << bitmap.height() << "stride:" << bitmap.stride();
    // QImage does not copy the buffer above, so copy before bitmap goes out of scope.
    image.save(base + "/summer.png", "PNG");
  }
  {
    document->applyStyleSheet(kWinterStyle);
    auto bitmap = document->renderToBitmap();
    // LunaSVG bitmap is premultiplied ARGB32 (BGRA byte order on little-endian).
    // Convert to a format Qt handles cleanly.
    bitmap.convertToRGBA(); // ensures RGBA byte order

    QImage image(bitmap.data(), bitmap.width(), bitmap.height(), bitmap.stride(),
                 QImage::Format_RGBA8888_Premultiplied);

    // QImage does not copy the buffer above, so copy before bitmap goes out of scope.
    image.save(base + "/winter.png", "PNG");
  }

  return app.exec();
}

/*  To Do
 *
 */
