#ifndef PREFERENCEMODEL_HPP
#define PREFERENCEMODEL_HPP

#include <QAbstractListModel>
#include <QColor>
#include <QFont>
#include <QList>

#include "../preferences_global.hpp"
#include "preference.hpp"

class PREFERENCE_EXPORT PreferenceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    // Define the role names to be used
    enum RoleNames {
        Categories  = Qt::UserRole + 1,
        General,
        Editor,
        Alu,
        FieldStart = Qt::UserRole + 10,//  Used for iteration.
        Id = FieldStart,
        Name,
        Parent,
        Type,
        Foreground,
        Background,
        FontBold,
        FontItalics,
        FontUnderline,
        FieldEnd,       //  Used for iteration, not exposed to QML
    };

    explicit PreferenceModel(QObject *parent = nullptr);
    ~PreferenceModel() = default;

    //  Data loaded on construction. This reloads
    //void reload();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Fetch data dynamically:
    QVariant data(const QModelIndex &index, int role = General) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    //void updateTestData();

protected:  //  Role Names must be under protected
    QHash<int, QByteArray> roleNames() const override;

private:
    void load();

    QHash<int, QByteArray>  roleNames_;
    QList<Preference>       preferences_;
    QFont                   currentFont_;
};

#endif // PREFERENCEMODEL_HPP
