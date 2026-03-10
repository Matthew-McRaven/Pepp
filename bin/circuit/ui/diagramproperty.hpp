#pragma once

#include <QMetaType> // Required for Q_DECLARE_METATYPE
#include <QObject>
#include <QRect>
#include <QSharedPointer>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "core/math/geom/rectangle.hpp"

#include "diagramtype.hpp"
using PeppRect = pepp::core::Rectangle<i16>;
using PeppSize = pepp::core::Size<i16>;
using PeppPt = pepp::core::Point<i16>;

struct DiagramProperty {

public:
  enum Role : u32 {
    Name = Qt::DisplayRole,
    Id = Qt::UserRole + 1,
    ImageSource,
    Type,
    InputNo,
    OutputNo,

    Selected,
    Orientation,
    Rectangle,

    //  Indicates invalid state from parsing input files
    Invalid = 0xffffffff,
  };

  bool setOrientation(const u32 v) {
    //  Limit to 360 degrees
    bool changed = false;
    const auto angle = v % 360;
    if (orientation != angle) {
      const auto slice = static_cast<u32>(angle / 90);

      //  Only support 90 degree changes
      orientation = slice * 90;

      changed = true;
    }
    return changed;
  }

  u32 id = 0;
  u32 orientation = 0; // Pointing Left

  //  Gate properties
  DiagramType::Type type = DiagramType::Invalid;
  u16 inputNo{2};
  u16 outputNo{1};

  //  Line properties
  u32 _start{0};
  u32 _finish{0};

  //  Diagram grid dimensions & placement
  PeppRect key{};
};

class DiagramProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(quint32 id READ id CONSTANT) // Read only
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString imageSource READ imageSource WRITE setImageSource NOTIFY imageChanged)
    Q_PROPERTY(DiagramType::Type type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(quint16 inputNo READ inputNo WRITE setInputNo NOTIFY inputChanged)
    Q_PROPERTY(quint16 outputNo READ outputNo WRITE setOutputNo NOTIFY outputChanged)
    Q_PROPERTY(quint16 orientation READ orientation WRITE setOrientation NOTIFY imageChanged)

  public:
    explicit DiagramProperties(QObject *parent = nullptr);

    QVariant get(int role) const;
    void set(int role, const QVariant &data);

    //  Data functions
    quint32 id() const { return _properties->id; } //  Unique object id
    DiagramType::Type type() const { return _properties->type; }
    quint16 inputNo() const { return _properties->inputNo; }
    quint16 outputNo() const { return _properties->outputNo; }
    void setId(const quint32 v);
    void setType(const DiagramType::Type v);
    void setInputNo(const quint16 v);
    void setOutputNo(const quint16 v);

    // Display functions
    QString name() const { return _name; }
    QString imageSource() const { return _imageSrc; }
    QPixmap *image() const { return _pixMap; }

    void setName(const QString v);
    void setImageSource(const QString v);

    bool selected() const { return _isSelected; }
    void setSelected(const bool v);
    void setImage(QPixmap *v);

    int orientation() const { return _properties->orientation; }
    void setOrientation(const quint32 v);

    const PeppRect &key() const { return _properties->key; }
    void setKey(const PeppRect &v);
    const PeppRect &gridRectangle() const { return _gridRect; }
    void setGridRectangle(const PeppRect &v);

  signals:
    void typeChanged();
    void nameChanged();
    void imageChanged();
    void inputChanged();
    void outputChanged();
    void selectedChanged();
    void dimensionsChanged();

  private:
    QSharedPointer<DiagramProperty> _properties;

    //  Display properties properties
    QString _name;
    QString _imageSrc;

    //  Presentation variables that require Qt stay in this class
    QPixmap *_pixMap = nullptr;

    //  Selection logic
    bool _isSelected = false;

    //  Display dimensions & placement
    PeppRect _gridRect{};
};
