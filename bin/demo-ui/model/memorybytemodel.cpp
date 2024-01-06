#include "memorybytemodel.h"

#include <cmath>

#include <QBrush>
#include <QColor>
#include <QPoint>
#include <QRect>
//  For testing only
#include <QRandomGenerator>


MemoryByteModel::MemoryByteModel(QObject *parent,
                                 const quint32 totalBytes,
                                 const quint8 bytesPerRow)
    : QAbstractTableModel(parent)
    ,   size_(totalBytes)
    ,   newData_(new quint8[totalBytes])
    ,   oldData_(new quint8[totalBytes])
    ,   column_(new MemoryColumns)
{
    //  Changing width also changes height
    setNumBytesPerLine( bytesPerRow);

    roleNames_[Byte]            =  "byteRole";
    roleNames_[Selected]        =  "selectedRole";
    roleNames_[Editing]         =  "editRole";
    roleNames_[TextColor]       =  "textColorRole";
    roleNames_[BackgroundColor] =  "backgroundColorRole";
    roleNames_[Qt::ToolTip]     =  "toolTipRole";
    roleNames_[Qt::TextAlignmentRole] =  "textAlignRole";
    roleNames_[Border]          =  "borderRole";

    clear();

    //  Test last cell
    //writeByte(size_ -1, 88);
}

quint8 MemoryByteModel::readByte(const quint32 address) const
{
    if( address > size_ )
        return 0;
    return newData_[address];
}

void MemoryByteModel::writeByte(const quint32 address, const quint8 value)
{
    //  Check for memmory overflow
    if( address >= size_ )
        return;

    //  Memory location is not same as location in model.
    //  Adjust address to match model address. QML model is offset 2 full
    //  columns due to line number and empty rectangle used for line
    const auto index = memoryIndex(address);

    //  Write byte to model
    setData(index, value, RoleNames::Byte);
}

void MemoryByteModel::setNumBytesPerLine(const quint8 bytesPerLine)
{
    Q_ASSERT( bytesPerLine > 0);

    //  Set bytes per row
    //  Initialized on construction to 8 bytes per row.
    //  If values are invalid, default is used
    if( bytesPerLine == 0 ) {
        width_ = 8;
    } else {
        //  Limit size to 32 since screen refresh will be slow
        width_ = bytesPerLine > 32 ? 32 : bytesPerLine;
    }

    //  Compute memory height.
    height_ = size_ / width_;

    //  Pad last row if not exactly divisible
    if((size_ % width_) != 0)
        ++height_;

    //  Updated column identifiers for width change
    column_->setNumBytesPerLine(width_);

    //  Signal that row count has changed
    emit dimensionsChanged();
}

QHash<int, QByteArray> MemoryByteModel::roleNames() const
{
    return roleNames_;
}

void MemoryByteModel::updateTestData()
{
    //  Disable auto update for development
    return;

    quint8 data{};
    //  Limit to first 8 rows
    const int row = static_cast<quint8>( QRandomGenerator::global()->generate() ) % 8;

    //  Update first 8 bytes with random data
    for( int i = 0; i < width_; ++i)
    {
        //  Create random data
        data = static_cast<quint8>( QRandomGenerator::global()->generate() );

        writeByte(row * width_ + i, data);
    }
}

QVariant MemoryByteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

int MemoryByteModel::rowCount(const QModelIndex &parent) const
{
    //  Number of rows of binary numbers
    return height_;
}

int MemoryByteModel::columnCount(const QModelIndex &parent) const
{
    //  Number of binary numbers in row plus row number and ascii representation
    Q_ASSERT( column_->Total() == (width_ + 4));
    return column_->Total();
}

QVariant MemoryByteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // The index returns the requested row and column information
    // The first column is the line number, which we ignore
    const int row = index.row();
    const int col = index.column();
    const int i   = memoryOffset(index);

    switch(role) {

    case Byte:
        //  First column is formatted row number
        if(col == column_->LineNo())
            return QStringLiteral("%1").arg(row * width_, 4, 16, QLatin1Char('0')).toUpper();

        //  Last columnis ascii representation of data
        if(col == column_->Ascii())
            return ascii(row);

        if(col == column_->Border1()  || col == column_->Border2())
            return QVariant("");

        if(i < 0)
            return QVariant("");

        //  Show data in hex format
        return QStringLiteral("%1").arg(newData_[i], 2, 16, QLatin1Char('0')).toUpper();
    case Selected:
        return false; //selected(index);
    case Editing:
        //  for last line when memory model is smaller than displayed items
        if(i < 0)
            return QVariant();

        //  Only one cell can be edited at a time
        return editing_;
    case TextColor:
        //  Set editing color if in edit mode
        if( editing_ > -1 && i == editing_ )
            return QVariant("#FF9800"); //  Pepperdine Orange

        //  For alternating columns, set color
        if( (col % 2) == 1 && col < column_->Border2())
            return QVariant("black");

        //  Default color
        return QVariant("black");
    case BackgroundColor:
        if(col == column_->Border1()  || col == column_->Border2())
            return QVariant("black");

        //  Handle invalid index
        if(i < 0 && row != 0)
            return QVariant("white");

        //  Set editing color if in edit mode
        if( editing_ > -1 && i == editing_ )
            return QVariant("#3F51B5"); //  Pepperdine Blue

        //  For alternating columns, set color
        //  for last line when memory model is smaller than displayed items
        if( (col % 2) == 1 && col < column_->Border2() && i < size_)
            return QVariant("#f0f0f0");

        //  Default color
        return QVariant("white");
    case Qt::TextAlignmentRole:
        if(col == column_->Ascii())
             return QVariant ( Qt::AlignLeft );

        //  Default for all other cells
        return QVariant(Qt::AlignHCenter);
    case Qt::ToolTip:
        //  Handle invalid index
        if(i < 0)
            return QVariant("");

        //return QVariant("tool tip");
        //  Only show for memory
        if(col >= column_->CellStart() && col <= column_->CellEnd()) {

            //  toUpper will work on entire string literal. Separate hex
            //  values and cast to upper case as separate strings
            const auto mem = QStringLiteral("%1")
                       .arg(i, 4, 16, QLatin1Char('0')).toUpper();
            const auto newH = QStringLiteral("%1")
                       .arg(newData_[i], 2, 16, QLatin1Char('0')).toUpper();
            const auto oldH = QStringLiteral("%1")
                       .arg(oldData_[i], 2, 16, QLatin1Char('0')).toUpper();

            return QStringLiteral("<b>Memory Location: 0x%1</b><br>"
                                  "Hex: 0x%2<br>"
                                  "Unsigned Decimal: %3<br>"
                                  "Binary: 0b%4<br>"
                                  "Previous Hex: 0x%5<br>"
                                  "Previous Unsigned Decimal: %6<br>"
                                  "Previous Binary: 0b%7")
                .arg(mem)                                   // Hex Address
                .arg(newH)                                  // Hex
                .arg(newData_[i])                           // Decimal
                .arg(newData_[i], 8, 2, QLatin1Char('0'))   // Binary
                .arg(oldH)                                  // Hex
                .arg(oldData_[i])                           // Decimal
                .arg(oldData_[i], 8, 2, QLatin1Char('0'));  // Binary
        }
        else
            return QVariant("");
    }

    return QVariant();
}

bool MemoryByteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //  See if value is different from passed in value
    switch (role) {
    case Selected: {
        const auto current = data(index, role);
        if( current != value) {
            const int i   = memoryOffset(index);
            if( i < 0)
                return false;

            //  Initial state before change
            const bool flag = selected_.contains(i);

            //  Add or remove based on existing value
            if(flag) {
                selected_.remove(i);
            } else {
                selected_.insert(i);
            }

            emit dataChanged(index, index);

            //  Flag value has been flipped
            return !flag;
        }
        break;
    }
    case Editing: {
        //const auto current = data(index, role);
        //if( current != value) {
            const int i   = memoryOffset(index);

            //  Bad index, just return
            if(i < 0)
                return false;

            QModelIndex ascii = QAbstractItemModel::createIndex(index.row(), column_->Ascii());

            //  Save index for editing
            editing_ = value.toInt();

            //  Repaint changed value
            emit dataChanged(index, ascii);

            //  Whenever a hex value is updated, also update ascii column
            //emit dataChanged(ascii, ascii);

            //  Return true if cleared
            return true;    //editing_ < 0;
        //}
        //break;
    }
    case Byte: {

        int hex = (int)std::strtol(value.toString().toStdString().c_str(), NULL, 16);
        const auto current = data(index, role);
        if( current != hex) {
            const int i   = memoryOffset(index);

            //  Bad index, just return
            if(i < 0)
                return false;

            QModelIndex ascii = QAbstractItemModel::createIndex(index.row(), column_->Ascii());

            //  Save old value
            oldData_[i] = newData_[i];

            //  Update new value
            newData_[i] = hex;

            //  Repaint changed value
            emit dataChanged(index, index);

            //  Whenever a hex value is updated, also update ascii column
            emit dataChanged(ascii, ascii);
        }
    }
    }

    return false;
}

Qt::ItemFlags MemoryByteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() ||
        index.column() < column_->CellStart() ||
        index.column() > column_->CellEnd())
        return Qt::NoItemFlags;

    //  All other items can be edited.
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void MemoryByteModel::clear()
{
    //  Clear memory buffers
    for(auto i = 0; i < size_; ++i) {
        newData_[i] = 0;
        oldData_[i] = 0;
    }

    emit dataChanged(index(0, 0), index(height_ - 1, column_->Ascii()));
}

//  Convert from cellIndex to memory location
//  Model is calculated as (col, row)
std::size_t MemoryByteModel::memoryOffset(const QModelIndex &index) const
{
    const std::size_t offset = index.row() * width_ + (index.column() - column_->CellStart());
    if(offset >= size_)
        return -1;

        //  Test if index is inside data model
    if( index.row() < 0 && index.row() >= height_) return -1;

    //  First column is line number. Skip
    if( index.column() <= 0 && index.column() >= width_ ) return -1;

    //  First column is line number. Return cell index to first edit column
    return offset;
}

//  Convert to location in model from memory location
QModelIndex MemoryByteModel::memoryIndex(std::size_t index)
{
    //  Check for memory overflow
    if( index >= size_ )
        return QModelIndex();

    const int row = std::floor(index / width_);
    const int col = index % width_;

    //  Test if index is inside data model
    if( row < 0 && row >= height_) return QModelIndex();

    //  First column is line number. Skip
    if( col <= 0 && col >= width_ ) return QModelIndex();

    //  First column is line number. Ignore
    return QAbstractItemModel::createIndex(row, col + column_->CellStart());
}

QString MemoryByteModel::ascii(const int row) const
{
    const int start = row * width_;
    const int end   = start + width_;
    QString edit;
    for( int i = start; i < end; ++i) {
        //  Use spaces for out of bound memory access
        //  Keeps data aligned in control
        if(i >= size_) {
            edit.append(" ");
            continue;
        }

        auto ascii = newData_[i];
        QChar c(ascii);

        if( c.isPrint()) {
            edit.append(c);
        } else {
            edit.append(".");
        }
    }
    return edit;
}

QVariant MemoryByteModel::selected(const QModelIndex& index, const RoleNames role) const
{
    if( !index.isValid())
        return QVariant();

    //  Convert to memory location
    int i = memoryOffset(index);
    if( i < 0)
        return false;

    //  Check for edit mode
    if( role == RoleNames::Editing)
        return editing_;
    else if( role == RoleNames::Selected)
        //  Return indicator for selected
        return selected_.contains(i);
    return QVariant();
}

QVariant MemoryByteModel::setSelected(const QModelIndex& index, const RoleNames role)
{
    //  Current field is not editable or selectable
    if( flags(index) == Qt::NoItemFlags)
        return false;

    //  Check for edit mode
    if( role == RoleNames::Editing) {
        //  New location
        const int i = memoryOffset(index);
        QModelIndex old;

        //  Clear old value, if any
        if( editing_ > -1 && i != editing_) {
            //  Save old index
            old = memoryIndex(editing_);
        }
        //  Set new value - changes formatting
        //  Remove selecte indicator if present
        selected_.remove(i);
        setData(index, i, role);

        //  remove formatting from old location
        if( old.isValid()) {
            emit dataChanged(old, old);
        }
        return editing_ < 0;
    }
    else if( role == RoleNames::Selected) {
        //  Return indicator for selected
        const auto flag = selected(index).toBool();

        //  Flip bit
        return setData(index, !flag, role);
    }

    //  Other roles are read only
    return QVariant();
}

void MemoryByteModel::clearSelected(const QModelIndex& index, const RoleNames role)
{
    //  Check for edit mode
    if( role == RoleNames::Editing && editing_ > -1) {
        //  Only 1 cell can be edited. Find cell from currently selected item
        auto old = memoryIndex(editing_);

        //  Fix colors on old cell
        setData(old, -1, role);
    }
}
