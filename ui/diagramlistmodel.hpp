#pragma once

#include <QAbstractListModel>
#include <Qt>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT
#include <QtQmlIntegration>

#include "diagramtype.hpp"

class DiagramTemplate : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(DiagramType::Type key READ key CONSTANT) // Read only
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString diagramType READ diagramType CONSTANT)
    Q_PROPERTY(QString qrcFile READ qrcFile CONSTANT)
    Q_PROPERTY(QString file READ file CONSTANT)

    DiagramType::Type _key{DiagramType::Invalid};
    QString _name;
    QString _diagramType;
    QString _qrcFile;
    QString _file;

public:
    explicit DiagramTemplate(QObject *parent = nullptr);
    DiagramTemplate(DiagramType::Type key,
                    QString name,
                    QString type,
                    QString qrc,
                    QString file,
                    QObject *parent = nullptr);
    DiagramType::Type key() const { return _key; }
    QString name() const { return _name; }
    QString diagramType() const { return _diagramType; }
    QString qrcFile() const { return _qrcFile; }
    QString file() const { return _file; }
};

class DiagramListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    //Q_PROPERTY(DiagramProperty *currentDiagram READ currentDiagram WRITE setCurrentDiagram NOTIFY
    //               diagramChanged)

    QList<DiagramTemplate *> _diagrams;

public:
    enum Role {
        Name = Qt::DisplayRole,
        Key = Qt::UserRole + 1,
        DiagramType,
        QrcFile,
        File,
    };
    explicit DiagramListModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE DiagramTemplate *diagramTemplate(int index) const
    {
        if (0 <= index && index < _diagrams.size()) {
            //emit diagramTypeChanged();
            return _diagrams.at(index);
        }

        //  Index is invalid rate
        return nullptr;
    }
};

class FilterDiagramListModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Filter filter READ filterGroupType WRITE setFilterGroupFilter NOTIFY filterChanged);
    Q_PROPERTY(DiagramListModel *model READ model WRITE setModel NOTIFY modelChanged);

public:
    enum Filter { Arrow = Qt::UserRole + 1, Diagram, Line, None = 0xffffffff };
    Q_ENUM(Filter)

    explicit FilterDiagramListModel(QObject *parent = nullptr);

    // Basic functionality:
    Filter filterGroupType() const { return _filter; }
    void setFilterGroupFilter(Filter filter = Filter::None);
    DiagramListModel *model() const { return static_cast<DiagramListModel *>(sourceModel()); }
    void setModel(DiagramListModel *model = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    //bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

signals:
    void filterChanged();
    void modelChanged();

private:
    Filter _filter = Filter::None;
    QString _filterString;
    //DiagramListModel *_model = nullptr;
};
