#pragma once

#include <QImage>
#include <QQuickPaintedItem>

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.

class QImage;

class GraphicCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GraphicCanvas)

    // Sizes in "screen" coordinates
    Q_PROPERTY(
        float contentWidth READ contentWidth WRITE setContentWidth NOTIFY dimensionsChanged FINAL)
    Q_PROPERTY(float contentHeight READ contentHeight WRITE setContentHeight NOTIFY
                   dimensionsChanged FINAL)

    // In "screen" coordinates (e.g., pixels according to our containing Flickable)
    Q_PROPERTY(float originX READ originX WRITE setOriginX NOTIFY originChanged FINAL)
    Q_PROPERTY(float originY READ originY WRITE setOriginY NOTIFY originChanged FINAL)

public:
    GraphicCanvas(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;

    // TODO: determine min/max bounds based on contained rectangles, then add padding around the edges.
    float contentWidth() const { return _dimensions.width(); }
    float contentHeight() const { return _dimensions.height(); }
    void setContentWidth(float v)
    {
        _dimensions.setWidth(v);
        emit dimensionsChanged();
    }
    void setContentHeight(float v)
    {
        _dimensions.setHeight(v);
        emit dimensionsChanged();
    }

    // The top-left corner, as measured in "screen" coordinates
    float originX() const { return _top_left.x() * grid_to_px; }
    float originY() const { return _top_left.y() * grid_to_px; }
    // Compute grid coordinates from screen coordinates
    void setOriginX(float x)
    {
        _top_left = {(x / grid_to_px), _top_left.y()};
        emit originChanged();
        update();
    }
    void setOriginY(float y)
    {
        _top_left = {_top_left.x(), (y / grid_to_px)};
        emit originChanged();
        update();
    }
signals:
    void xBoundsChanged();
    void yBoundsChanged();
    void originChanged();
    void dimensionsChanged();

private:
    // Magic constant to convert from grid coordinates to screen coordinates
    const float grid_to_px = 4.0f;
    const int block_size = 25;

    // One of the classes from my geometry library. See core/math/geom
    //using Rectangle = pepp::core::Rectangle<i16>;
    // Helepr for painting a single rect that has already "passed" the clipping test.
    //void paint_one(QPainter *painter, QRect rect, void *props);
    QRectF grid_to_screen(QRectF rect);
    QRectF screen_to_grid(QRectF rect);

    //  Render and cache images for painting
    void cacheImages(const QString &source);

    // The things we want to render
    std::vector<QRect> _rects;
    QList<QImage> _svgs;

    // Top-left corner of the viewport in grid coordinates
    QPointF _top_left{};
    QSizeF _dimensions{320, 320};
};
