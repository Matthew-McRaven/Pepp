#pragma once

#include <QPixmap>
#include <QQuickPaintedItem>

#include "diagramdatamodel.hpp"
#include "diagramlistmodel.hpp"
#include "diagramproperty.hpp"

class DiagramDataModel;

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.
class GraphicCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GraphicCanvas)

    // Sizes in "screen" coordinates
    Q_PROPERTY(float contentWidth READ contentWidth NOTIFY boundsChanged FINAL)
    Q_PROPERTY(float contentHeight READ contentHeight NOTIFY boundsChanged FINAL)

    // In "screen" coordinates (e.g., pixels according to our containing Flickable)
    Q_PROPERTY(float originX READ originX WRITE setOriginX NOTIFY originChanged FINAL)
    Q_PROPERTY(float originY READ originY WRITE setOriginY NOTIFY originChanged FINAL)

    // Used to shrink canvas to fit in scroll bars
    Q_PROPERTY(float xScrollbar READ xScrollbar WRITE setXScrollbar NOTIFY boundsChanged FINAL)
    Q_PROPERTY(float yScrollbar READ yScrollbar WRITE setYScrollbar NOTIFY boundsChanged FINAL)

    //  Set and access datamodel and template
    Q_PROPERTY(DiagramDataModel *model READ model WRITE setModel NOTIFY boundsChanged FINAL)
    Q_PROPERTY(DiagramTemplate *template READ stamp WRITE setStamp NOTIFY stampChanged FINAL)

public:
    GraphicCanvas(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;

    // Max bounds based on contained rectangles.
    float contentWidth() const { return _dimensions.width() * grid_to_px; }
    float contentHeight() const { return _dimensions.height() * grid_to_px; }

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

    // The top-left corner, as measured in "screen" coordinates
    float xScrollbar() const { return _scrollbarWidth.right() * grid_to_px; }
    float yScrollbar() const { return _scrollbarWidth.bottom() * grid_to_px; }

    // Compute grid coordinates from screen coordinates
    void setXScrollbar(float x)
    {
        if (std::abs(x - _scrollbarWidth.right()) > .0001) {
            _scrollbarWidth.setRight(x);
            emit boundsChanged();
            update();
        }
    }
    void setYScrollbar(float y)
    {
        if (std::abs(y - _scrollbarWidth.bottom()) > .0001) {
            _scrollbarWidth.setBottom(y);
            emit boundsChanged();
            update();
        }
    }

    DiagramDataModel *model() const { return _model; }
    void setModel(DiagramDataModel *model)
    {
        if (model != _model) {
            _model = model;
            emit modelChanged();
            update();
        }
    }

    DiagramTemplate *stamp() const { return _template; }
    void setStamp(DiagramTemplate *stamp)
    {
        if (stamp && stamp != _template) {
            //  Is valid stamp
            if (stamp->diagramType() == "Diagram") {
                _template = stamp;
            } else {
                _template = nullptr;
            }

            //  Changing template only affects current item to stamp down
            //  Does not require a redraw
            emit stampChanged();
            //update();
        }
    }

protected:
    //  Mouse events
    //void mouseDoubleClickEvent(QMouseEvent *event) override;
    //void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    /*void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;

    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;*/

    void wheelEvent(QWheelEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
signals:
    void boundsChanged();
    void modelChanged();
    void originChanged();
    void stampChanged();

private:
    // Magic constant to convert from grid coordinates to screen coordinates
    // Zoom variables
    qreal grid_to_px = 4.0;
    const qreal _minScale = 0.25;
    const qreal _maxScale = 16.0;
    qreal _currentZoom = 1.0;

    //  Grid dimensions (logical size, screen size is this times grid_to_px
    const int block_size = 25;

    //  Centralize image addition so we can track canvas size
    void insertImage(const QRect &rect, DiagramProperties *data);

    // Helepr for painting a single rect that has already "passed" the clipping test.
    void paint_one(QPainter *painter, QRect rect, const DiagramProperties &props);
    QRectF grid_to_screen(QRectF rect);
    QRectF screen_to_grid(QRectF rect);
    QPoint screen_to_grid(QPointF point);

    void setZoom(qint8 change);

    //  Sets currently selected diagram
    bool setSelected(const QPoint);

    //  Render and cache images for painting
    void cacheImages(const QString &source);

    //  Insert test data
    void updateData();

    //  Respond to data changes in model
    void updateCell(const QModelIndex &from, const QModelIndex &to);

    // The things we want to render
    using Rect = std::pair<QRect, DiagramProperties *>;
    std::vector<Rect> _rects;

    //  Cached images
    QList<QPixmap> _svgs;
    QList<QPixmap> _svgsBottom;
    QList<QPixmap> _svgsLeft;
    QList<QPixmap> _svgsTop;

    // Top-left corner of the viewport in grid coordinates
    QPointF _top_left{};
    QRectF _dimensions{0, 0, 25.0, 25.0};

    //  Margins are always in screen coordinates since they do not
    //  interact with the drawing model. They only impact screen clipping
    QMarginsF _scrollbarWidth{0, 0, 0, 0};

    //  Make fixed color for now
    QColor _highlight = QColorConstants::Svg::cornflowerblue;

    //  Data model
    DiagramDataModel *_model = nullptr;
    DiagramTemplate *_template = nullptr;
};
