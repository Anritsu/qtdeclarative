/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmltablemodel_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTableModel, "qt.qml.tablemodel")

static const QString lengthPropertyName = QStringLiteral("length");
static const QString displayRoleName = QStringLiteral("display");

/*!
    \qmltype TableModel
    \instantiates QQmlTableModel
    \inqmlmodule Qt.labs.qmlmodels
    \brief Encapsulates a simple table model.
    \since 5.12

    The TableModel type stores JavaScript objects as data for a table model
    that can be used with \l TableView.

    The following snippet shows the simplest use case for TableModel:

    \snippet qml/tablemodel/fruit-example-simpledelegate.qml file

    The model's initial data is set with either the \l rows property or by
    calling \l appendRow(). Once the first row has been added to the table, the
    columns and roles are established and will be fixed for the lifetime of the
    model.

    To access a specific row, the \l getRow() function can be used.
    It's also possible to access the model's JavaScript data
    directly via the \l rows property, but it is not possible to
    modify the model data this way.

    To add new rows, use \l appendRow() and \l insertRow(). To modify
    existing rows, use \l setRow(), \l moveRow(), \l removeRow(), and
    \l clear().

    It is also possible to modify the model's data via the delegate,
    as shown in the example above:

    \snippet qml/tablemodel/fruit-example-simpledelegate.qml delegate

    If the type of the data at the modified role does not match the type of the
    data that is set, it will be automatically converted via
    \l {QVariant::canConvert()}{QVariant}.

    For convenience, TableModel provides the \c display role if it is not
    explicitly specified in any column. When a column only has one role
    declared, that role will be used used as the display role. However, when
    there is more than one role in a column, which role will be used is
    undefined. This is because JavaScript does not guarantee that properties
    within an object can be accessed according to the order in which they were
    declared. This is why \c checkable may be used as the display role for the
    first column even though \c checked is declared before it, for example.

    \section1 Using DelegateChooser with TableModel

    For most real world use cases, it is recommended to use DelegateChooser
    as the delegate of a TableView that uses TableModel. This allows you to
    use specific roles in the relevant delegates. For example, the snippet
    above can be rewritten to use DelegateChooser like so:

    \snippet qml/tablemodel/fruit-example-delegatechooser.qml file

    The most specific delegates are declared first: the columns at index \c 0
    and \c 1 have \c bool and \c integer data types, so they use a
    \l [QtQuickControls2]{CheckBox} and \l [QtQuickControls2]{SpinBox},
    respectively. The remaining columns can simply use a
    \l [QtQuickControls2]{TextField}, and so that delegate is declared
    last as a fallback.

    \sa QAbstractTableModel, TableView
*/

QQmlTableModel::QQmlTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    mRoleNames = QAbstractTableModel::roleNames();
}

QQmlTableModel::~QQmlTableModel()
{
}

/*!
    \qmlproperty object TableModel::rows

    This property holds the model data in the form of an array of rows:

    \snippet qml/tablemodel/fruit-example-simpledelegate.qml rows

    \sa getRow(), setRow(), moveRow(), appendRow(), insertRow(), clear(), rowCount, columnCount
*/
QVariant QQmlTableModel::rows() const
{
    return mRows;
}

void QQmlTableModel::setRows(const QVariant &rows)
{
    if (rows.userType() != qMetaTypeId<QJSValue>()) {
        qmlWarning(this) << "setRows(): \"rows\" must be an array; actual type is " << rows.typeName();
        return;
    }

    const QJSValue rowsAsJSValue = rows.value<QJSValue>();
    const QVariantList rowsAsVariantList = rowsAsJSValue.toVariant().toList();
    if (rowsAsVariantList == mRows) {
        // No change.
        return;
    }

    QVariant firstRowAsVariant;
    QVariantList firstRow;
    if (!rowsAsVariantList.isEmpty()) {
        // There are rows to validate. If they're not valid,
        // we'll return early without changing anything.
        firstRowAsVariant = rowsAsVariantList.first();
        firstRow = firstRowAsVariant.toList();

        if (firstRowAsVariant.type() != QVariant::List) {
            qmlWarning(this) << "setRows(): each row in \"rows\" must be an array of objects";
            return;
        }

        if (mColumnCount > 0) {
            qCDebug(lcTableModel) << "validating" << rowsAsVariantList.size()
                << "rows against existing metadata";

            // This is not the first time the rows have been set; validate the new columns.
            for (int i = 0; i < rowsAsVariantList.size(); ++i) {
                // validateNewRow() expects a QVariant wrapping a QJSValue, so to
                // simplify the code, just create one here.
                const QVariant row = QVariant::fromValue(rowsAsJSValue.property(i));
                if (!validateNewRow("setRows()", row, i))
                    return;
            }
        }
    }

    const int oldRowCount = mRowCount;
    const int oldColumnCount = mColumnCount;

    beginResetModel();

    // We don't clear the column or role data, because a TableModel should not be reused in that way.
    // Once it has valid data, its columns and roles are fixed.
    mRows = rowsAsVariantList;
    mRowCount = mRows.size();

    const bool isFirstTimeSet = mColumnCount == 0;
    if (isFirstTimeSet && mRowCount > 0) {
        // This is the first time the rows have been set, so establish
        // the column count and gather column metadata.
        mColumnCount = firstRow.size();
        qCDebug(lcTableModel) << "gathering metadata for" << mColumnCount << "columns from first row:";

        // Go through each property of each cell in the first row
        // and make a role name from it.
        int userRoleKey = Qt::UserRole;
        for (int columnIndex = 0; columnIndex < mColumnCount; ++columnIndex) {
            // We need it as a QVariantMap because we need to get
            // the name of the property, which we can't do with QJSValue's API.
            const QVariantMap column = firstRow.at(columnIndex).toMap();
            const QStringList columnPropertyNames = column.keys();
            ColumnProperties properties;
            int propertyInfoIndex = 0;

            qCDebug(lcTableModel).nospace() << "- column " << columnIndex << ":";

            for (const QString &roleName : columnPropertyNames) {
                // QML/JS supports utf8.
                const QByteArray roleNameUtf8 = roleName.toUtf8();
                if (!mRoleNames.values().contains(roleNameUtf8)) {
                    // We don't already have this role name, so it's a user role.
                    mRoleNames[userRoleKey] = roleName.toUtf8().constData();
                    qCDebug(lcTableModel) << "  - added new user role" << roleName << "with key" << userRoleKey;
                    ++userRoleKey;
                } else {
                    qCDebug(lcTableModel) << "  - found existing role" << roleName;
                }

                if (properties.explicitDisplayRoleIndex == -1 && roleName == displayRoleName) {
                    // The user explicitly declared a "display" role,
                    // so now we don't need to make it the first role in the column for them.
                    properties.explicitDisplayRoleIndex = propertyInfoIndex;
                }

                // Keep track of the type of property so we can use it to validate new rows later on.
                const QVariant roleValue = column.value(roleName);
                const auto propertyInfo = ColumnPropertyInfo(roleName, roleValue.type(),
                    QString::fromLatin1(roleValue.typeName()));
                properties.infoForProperties.append(propertyInfo);

                qCDebug(lcTableModel) << "    - column property" << propertyInfo.name
                    << "has type" << propertyInfo.typeName;

                ++propertyInfoIndex;
            }

            mColumnProperties.append(properties);
        }
    }

    endResetModel();

    emit rowsChanged();

    if (mRowCount != oldRowCount)
        emit rowCountChanged();
    if (mColumnCount != oldColumnCount)
        emit columnCountChanged();
}

/*!
    \qmlmethod TableModel::appendRow(object row)

    Adds a new row to the end of the model, with the
    values (cells) in \a row.

    \code
        model.appendRow([
            { checkable: true, checked: false },
            { amount: 1 },
            { fruitType: "Pear" },
            { fruitName: "Williams" },
            { fruitPrice: 1.50 },
        ])
    \endcode

    \sa insertRow(), setRow(), removeRow()
*/
void QQmlTableModel::appendRow(const QVariant &row)
{
    if (!validateNewRow("appendRow()", row, -1, AppendOperation))
        return;

    doInsert(mRowCount, row);
}

/*!
    \qmlmethod TableModel::clear()

    Removes all rows from the model.

    \sa removeRow()
*/
void QQmlTableModel::clear()
{
    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    setRows(QVariant::fromValue(engine->newArray()));
}

/*!
    \qmlmethod object TableModel::getRow(int rowIndex)

    Returns the row at \a rowIndex in the model.

    Note that this equivalent to accessing the row directly
    through the \l rows property:

    \code
    Component.onCompleted: {
        // These two lines are equivalent.
        console.log(model.getRow(0).fruitName);
        console.log(model.rows[0].fruitName);
    }
    \endcode

    \note the returned object cannot be used to modify the contents of the
    model; use setRow() instead.

    \sa setRow(), appendRow(), insertRow(), removeRow(), moveRow()
*/
QVariant QQmlTableModel::getRow(int rowIndex)
{
    if (!validateRowIndex("getRow()", "rowIndex", rowIndex))
        return QVariant();

    return mRows.at(rowIndex);
}

/*!
    \qmlmethod TableModel::insertRow(int rowIndex, object row)

    Adds a new row to the list model at position \a rowIndex, with the
    values (cells) in \a row.

    \code
        model.insertRow(2, [
            { checkable: true, checked: false },
            { amount: 1 },
            { fruitType: "Pear" },
            { fruitName: "Williams" },
            { fruitPrice: 1.50 },
        ])
    \endcode

    The \a rowIndex must be to an existing item in the list, or one past
    the end of the list (equivalent to \l appendRow()).

    \sa appendRow(), setRow(), removeRow(), rowCount
*/
void QQmlTableModel::insertRow(int rowIndex, const QVariant &row)
{
    if (!validateNewRow("insertRow()", row, rowIndex))
        return;

    doInsert(rowIndex, row);
}

void QQmlTableModel::doInsert(int rowIndex, const QVariant &row)
{
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);

    // Adding rowAsVariant.toList() will add each invidual variant in the list,
    // which is definitely not what we want.
    const QVariant rowAsVariant = row.value<QJSValue>().toVariant();
    mRows.insert(rowIndex, rowAsVariant);
    ++mRowCount;

    qCDebug(lcTableModel).nospace() << "inserted the following row to the model at index"
        << rowIndex << ":\n" << rowAsVariant.toList();

    endInsertRows();
    emit rowCountChanged();
}

/*!
    \qmlmethod TableModel::moveRow(int fromRowIndex, int toRowIndex, int rows)

    Moves \a rows from the index at \a fromRowIndex to the index at
    \a toRowIndex.

    The from and to ranges must exist; for example, to move the first 3 items
    to the end of the list:

    \code
        model.moveRow(0, model.rowCount - 3, 3)
    \endcode

    \sa appendRow(), insertRow(), removeRow(), rowCount
*/
void QQmlTableModel::moveRow(int fromRowIndex, int toRowIndex, int rows)
{
    if (fromRowIndex == toRowIndex) {
        qmlWarning(this) << "moveRow(): \"fromRowIndex\" cannot be equal to \"toRowIndex\"";
        return;
    }

    if (rows <= 0) {
        qmlWarning(this) << "moveRow(): \"rows\" is less than or equal to 0";
        return;
    }

    if (!validateRowIndex("moveRow()", "fromRowIndex", fromRowIndex))
        return;

    if (!validateRowIndex("moveRow()", "toRowIndex", toRowIndex))
        return;

    if (fromRowIndex + rows > mRowCount) {
        qmlWarning(this) << "moveRow(): \"fromRowIndex\" (" << fromRowIndex
            << ") + \"rows\" (" << rows << ") = " << (fromRowIndex + rows)
            << ", which is greater than rowCount() of " << mRowCount;
        return;
    }

    if (toRowIndex + rows > mRowCount) {
        qmlWarning(this) << "moveRow(): \"toRowIndex\" (" << toRowIndex
            << ") + \"rows\" (" << rows << ") = " << (toRowIndex + rows)
            << ", which is greater than rowCount() of " << mRowCount;
        return;
    }

    qCDebug(lcTableModel).nospace() << "moving " << rows
        << " row(s) from index " << fromRowIndex
        << " to index " << toRowIndex;

    // Based on the same call in QQmlListModel::moveRow().
    beginMoveRows(QModelIndex(), fromRowIndex, fromRowIndex + rows - 1, QModelIndex(),
        toRowIndex > fromRowIndex ? toRowIndex + rows : toRowIndex);

    // Based on ListModel::moveRow().
    if (fromRowIndex > toRowIndex) {
        // Only move forwards - flip if moving backwards.
        const int from = fromRowIndex;
        const int to = toRowIndex;
        fromRowIndex = to;
        toRowIndex = to + rows;
        rows = from - to;
    }

    QVector<QVariant> store;
    store.reserve(rows);
    for (int i = 0; i < (toRowIndex - fromRowIndex); ++i)
        store.append(mRows.at(fromRowIndex + rows + i));
    for (int i = 0; i < rows; ++i)
        store.append(mRows.at(fromRowIndex + i));
    for (int i = 0; i < store.size(); ++i)
        mRows[fromRowIndex + i] = store[i];

    qCDebug(lcTableModel).nospace() << "after moving, rows are:\n" << mRows;

    endMoveRows();
}

/*!
    \qmlmethod TableModel::removeRow(int rowIndex, int rows = 1)

    Removes the row at \a rowIndex from the model.

    \sa clear(), rowCount
*/
void QQmlTableModel::removeRow(int rowIndex, int rows)
{
    if (!validateRowIndex("removeRow()", "rowIndex", rowIndex))
        return;

    if (rows <= 0) {
        qmlWarning(this) << "removeRow(): \"rows\" is less than or equal to zero";
        return;
    }

    if (rowIndex + rows - 1 >= mRowCount) {
        qmlWarning(this) << "removeRow(): \"rows\" " << rows
            << " exceeds available rowCount() of " << mRowCount
            << " when removing from \"rowIndex\" " << rowIndex;
        return;
    }

    beginRemoveRows(QModelIndex(), rowIndex, rowIndex + rows - 1);

    auto firstIterator = mRows.begin() + rowIndex;
    // The "last" argument to erase() is exclusive, so we go one past the last item.
    auto lastIterator = firstIterator + rows;
    mRows.erase(firstIterator, lastIterator);
    mRowCount -= rows;

    endRemoveRows();
    emit rowCountChanged();

    qCDebug(lcTableModel).nospace() << "removed " << rows
        << " items from the model, starting at index " << rowIndex;
}

/*!
    \qmlmethod TableModel::setRow(int rowIndex, object row)

    Changes the row at \a rowIndex in the model with \a row.

    All columns/cells must be present in \c row, and in the correct order.

    \code
        model.setRow(0, [
            { checkable: true, checked: false },
            { amount: 1 },
            { fruitType: "Pear" },
            { fruitName: "Williams" },
            { fruitPrice: 1.50 },
        ])
    \endcode

    If \a rowIndex is equal to \c rowCount(), then a new row is appended to the
    model. Otherwise, \a rowIndex must point to an existing row in the model.

    \sa appendRow(), insertRow(), rowCount
*/
void QQmlTableModel::setRow(int rowIndex, const QVariant &row)
{
    if (!validateNewRow("setRow()", row, rowIndex))
        return;

    if (rowIndex != mRowCount) {
        // Setting an existing row.
        mRows[rowIndex] = row;

        // For now we just assume the whole row changed, as it's simpler.
        const QModelIndex topLeftModelIndex(createIndex(rowIndex, 0));
        const QModelIndex bottomRightModelIndex(createIndex(rowIndex, mColumnCount - 1));
        emit dataChanged(topLeftModelIndex, bottomRightModelIndex);
    } else {
        // Appending a row.
        doInsert(rowIndex, row);
    }
}

/*!
    \qmlproperty var TableModel::roleDataProvider

    This property can hold a function that will map roles to values.

    When assigned, it will be called each time data() is called, to enable
    extracting arbitrary values, converting the data in arbitrary ways, or even
    doing calculations. It takes 3 arguments: \c index (\l QModelIndex),
    \c role (string), and \c cellData (object), which is the complete data that
    is stored in the given cell. (If the cell contains a JS object with
    multiple named values, the entire object will be given in \c cellData.)
    The function that you define must return the value to be used; for example
    a typical delegate will display the value returned for the \c display role,
    so you can check whether that is the role and return data in a form that is
    suitable for the delegate to show:

    \snippet qml/tablemodel/roleDataProvider.qml 0
*/
QJSValue QQmlTableModel::roleDataProvider() const
{
    return mRoleDataProvider;
}

void QQmlTableModel::setRoleDataProvider(QJSValue roleDataProvider)
{
    if (roleDataProvider.strictlyEquals(mRoleDataProvider))
        return;

    mRoleDataProvider = roleDataProvider;
    emit roleDataProviderChanged();
}

/*!
    \qmlmethod QModelIndex TableModel::index(int row, int column)

    Returns a \l QModelIndex object referencing the given \a row and \a column,
    which can be passed to the data() function to get the data from that cell,
    or to setData() to edit the contents of that cell.

    \code
    import QtQml 2.14
    import Qt.labs.qmlmodels 1.0

    TableModel {
        id: model
        rows: [
            [{ fruitType: "Apple" }, { fruitPrice: 1.50 }],
            [{ fruitType: "Orange" }, { fruitPrice: 2.50 }]
        ]
        Component.onCompleted: {
            for (var r = 0; r < model.rowCount; ++r) {
                console.log("An " + model.data(model.index(r, 0)).fruitType +
                            " costs " + model.data(model.index(r, 1)).fruitPrice.toFixed(2))
            }
        }
    }
    \endcode

    \sa {QModelIndex and related Classes in QML}, data()
*/
// Note: we don't document the parent argument, because you never need it, because
// cells in a TableModel don't have parents.  But it is there because this function is an override.
QModelIndex QQmlTableModel::index(int row, int column, const QModelIndex &parent) const
{
    return row >= 0 && row < rowCount() && column >= 0 && column < columnCount() && !parent.isValid()
        ? createIndex(row, column)
        : QModelIndex();
}

/*!
    \qmlproperty int TableModel::rowCount
    \readonly

    This read-only property holds the number of rows in the model.

    This value changes whenever rows are added or removed from the model.
*/
int QQmlTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mRowCount;
}

/*!
    \qmlproperty int TableModel::columnCount
    \readonly

    This read-only property holds the number of columns in the model.

    The number of columns is fixed for the lifetime of the model
    after the \l rows property is set or \l appendRow() is called for the first
    time.
*/
int QQmlTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mColumnCount;
}

/*!
    \qmlmethod variant TableModel::data(QModelIndex index, string role)

    Returns the data from the table cell at the given \a index belonging to the
    given \a role.

    \sa index()
*/
QVariant QQmlTableModel::data(const QModelIndex &index, const QString &role) const
{
    const int iRole = mRoleNames.key(role.toUtf8(), -1);
    if (iRole >= 0)
        return data(index, iRole);
    return QVariant();
}

QVariant QQmlTableModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= rowCount())
        return QVariant();

    const int column = index.column();
    if (column < 0 || column >= columnCount())
        return QVariant();

    if (!mRoleNames.contains(role))
        return QVariant();

    const QVariantList rowData = mRows.at(row).toList();

    if (mRoleDataProvider.isCallable()) {
        auto engine = qmlEngine(this);
        const auto args = QJSValueList() <<
            engine->toScriptValue(index) <<
            QString::fromUtf8(mRoleNames.value(role)) <<
            engine->toScriptValue(rowData.at(column));
        return const_cast<QQmlTableModel*>(this)->mRoleDataProvider.call(args).toVariant();
    }

    // TODO: should we also allow this code to be executed if roleDataProvider doesn't
    // handle the role/column, so that it only has to handle the case where there is
    // more than one role in a column?
    const QVariantMap columnData = rowData.at(column).toMap();
    const QString propertyName = columnPropertyNameFromRole(column, role);
    const QVariant value = columnData.value(propertyName);
    return value;
}

/*!
    \qmlmethod bool TableModel::setData(QModelIndex index, string role, variant value)

    Inserts or updates the data field named by \a role in the table cell at the
    given \a index with \a value. Returns true if sucessful, false if not.

    \sa index()
*/
bool QQmlTableModel::setData(const QModelIndex &index, const QString &role, const QVariant &value)
{
    const int iRole = mRoleNames.key(role.toUtf8(), -1);
    if (iRole >= 0)
        return setData(index, value, iRole);
    return false;
}

bool QQmlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const int row = index.row();
    if (row < 0 || row >= rowCount())
        return false;

    const int column = index.column();
    if (column < 0 || column >= columnCount())
        return false;

    if (!mRoleNames.contains(role))
        return false;

    const QVariantList rowData = mRows.at(row).toList();
    const QString propertyName = columnPropertyNameFromRole(column, role);

    qCDebug(lcTableModel).nospace() << "setData() called with index "
        << index << ", value " << value << " and role " << propertyName;

    // Verify that the role exists for this column.
    const ColumnPropertyInfo propertyInfo = findColumnPropertyInfo(column, propertyName);
    if (!propertyInfo.isValid()) {
        QString message;
        QDebug stream(&message);
        stream.nospace() << "setData(): no role named " << propertyName
            << " at column index " << column << ". The available roles for that column are:\n";

        const QVector<ColumnPropertyInfo> availableProperties = mColumnProperties.at(column).infoForProperties;
        for (auto propertyInfo : availableProperties)
            stream << "    - " << propertyInfo.name << " (" << qPrintable(propertyInfo.typeName) << ")";

        qmlWarning(this) << message;
        return false;
    }

    // Verify that the type of the value is what we expect.
    // If the value set is not of the expected type, we can try to convert it automatically.
    QVariant effectiveValue = value;
    if (value.type() != propertyInfo.type) {
        if (!value.canConvert(int(propertyInfo.type))) {
            qmlWarning(this).nospace() << "setData(): the value " << value
                << " set at row " << row << " column " << column << " with role " << propertyName
                << " cannot be converted to " << propertyInfo.typeName;
            return false;
        }

        if (!effectiveValue.convert(int(propertyInfo.type))) {
            qmlWarning(this).nospace() << "setData(): failed converting value " << value
                << " set at row " << row << " column " << column << " with role " << propertyName
                << " to " << propertyInfo.typeName;
            return false;
        }
    }

    QVariantMap modifiedColumn = rowData.at(column).toMap();
    modifiedColumn[propertyName] = value;

    QVariantList modifiedRow = rowData;
    modifiedRow[column] = modifiedColumn;
    mRows[row] = modifiedRow;

    QVector<int> rolesChanged;
    rolesChanged.append(role);
    emit dataChanged(index, index, rolesChanged);

    return true;
}

QHash<int, QByteArray> QQmlTableModel::roleNames() const
{
    return mRoleNames;
}

QQmlTableModel::ColumnPropertyInfo::ColumnPropertyInfo()
{
}

QQmlTableModel::ColumnPropertyInfo::ColumnPropertyInfo(
    const QString &name, QVariant::Type type, const QString &typeName) :
    name(name),
    type(type),
    typeName(typeName)
{
}

bool QQmlTableModel::ColumnPropertyInfo::isValid() const
{
    return !name.isEmpty();
}

bool QQmlTableModel::validateRowType(const char *functionName, const QVariant &row) const
{
    if (row.userType() != qMetaTypeId<QJSValue>()) {
        qmlWarning(this) << functionName << ": expected \"row\" argument to be an array,"
            << " but got " << row.typeName() << " instead";
        return false;
    }

    const QVariant rowAsVariant = row.value<QJSValue>().toVariant();
    if (rowAsVariant.type() != QVariant::List) {
        qmlWarning(this) << functionName << ": expected \"row\" argument to be an array,"
            << " but got " << row.typeName() << " instead";
        return false;
    }

    return true;
}

bool QQmlTableModel::validateNewRow(const char *functionName, const QVariant &row,
    int rowIndex, NewRowOperationFlag appendFlag) const
{
    if (!validateRowType(functionName, row))
        return false;

    if (appendFlag == OtherOperation) {
        // Inserting/setting.
        if (rowIndex < 0) {
            qmlWarning(this) << functionName << ": \"rowIndex\" cannot be negative";
            return false;
        }

        if (rowIndex > mRowCount) {
            qmlWarning(this) << functionName << ": \"rowIndex\" " << rowIndex
                << " is greater than rowCount() of " << mRowCount;
            return false;
        }
    }

    const QVariant rowAsVariant = row.value<QJSValue>().toVariant();
    const QVariantList rowAsList = rowAsVariant.toList();

    const int columnCount = rowAsList.size();
    if (columnCount != mColumnCount) {
        qmlWarning(this) << functionName << ": expected " << mColumnCount
            << " columns, but got " << columnCount;
        return false;
    }

    // Verify that the row's columns and their roles match the name and type of existing data.
    // This iterates across the columns in the row. For example:
    // [
    //     { checkable: true, checked: false },   // columnIndex == 0
    //     { amount: 1 },                         // columnIndex == 1
    //     { fruitType: "Orange" },               // etc.
    //     { fruitName: "Navel" },
    //     { fruitPrice: 2.50 }
    // ],
    for (int columnIndex = 0; columnIndex < mColumnCount; ++columnIndex) {
        const QVariantMap column = rowAsList.at(columnIndex).toMap();
        if (!validateColumnPropertyTypes(functionName, column, columnIndex))
            return false;
    }

    return true;
}

bool QQmlTableModel::validateRowIndex(const char *functionName, const char *argumentName, int rowIndex) const
{
    if (rowIndex < 0) {
        qmlWarning(this) << functionName << ": \"" << argumentName << "\" cannot be negative";
        return false;
    }

    if (rowIndex >= mRowCount) {
        qmlWarning(this) << functionName << ": \"" << argumentName
            << "\" " << rowIndex << " is greater than or equal to rowCount() of " << mRowCount;
        return false;
    }

    return true;
}

bool QQmlTableModel::validateColumnPropertyTypes(const char *functionName,
    const QVariantMap &column, int columnIndex) const
{
    // Actual
    const QVariantList columnProperties = column.values();
    const QStringList propertyNames = column.keys();
    // Expected
    const QVector<ColumnPropertyInfo> properties = mColumnProperties.at(columnIndex).infoForProperties;

    // This iterates across the properties in the column. For example:
    //   0         1       2
    // { foo: "A", bar: 1, baz: true },
    for (int propertyIndex = 0; propertyIndex < properties.size(); ++propertyIndex) {
        const QString propertyName = propertyNames.at(propertyIndex);
        const QVariant propertyValue = columnProperties.at(propertyIndex);
        const ColumnPropertyInfo expectedPropertyFormat = properties.at(propertyIndex);

        if (!validateColumnPropertyType(functionName, propertyName,
                propertyValue, expectedPropertyFormat, columnIndex)) {
            return false;
        }
    }

    return true;
}

bool QQmlTableModel::validateColumnPropertyType(const char *functionName, const QString &propertyName,
    const QVariant &propertyValue, const ColumnPropertyInfo &expectedPropertyFormat, int columnIndex) const
{
    if (propertyName != expectedPropertyFormat.name) {
        qmlWarning(this) << functionName
            << ": expected property named " << expectedPropertyFormat.name
            << " at column index " << columnIndex
            << ", but got " << propertyName << " instead";
        return false;
    }

    if (propertyValue.type() != expectedPropertyFormat.type) {
        qmlWarning(this) << functionName
            << ": expected property with type " << expectedPropertyFormat.typeName
            << " at column index " << columnIndex
            << ", but got " << propertyValue.typeName() << " instead";
        return false;
    }

    return true;
}

QQmlTableModel::ColumnPropertyInfo QQmlTableModel::findColumnPropertyInfo(
    int columnIndex, const QString &columnPropertyName) const
{
    // TODO: check if a hash with its string-based lookup is faster,
    // keeping in mind that we may be doing index-based lookups too.
    const QVector<ColumnPropertyInfo> properties = mColumnProperties.at(columnIndex).infoForProperties;
    for (int i = 0; i < properties.size(); ++i) {
        const ColumnPropertyInfo &info = properties.at(i);
        if (info.name == columnPropertyName)
            return info;
    }

    return ColumnPropertyInfo();
}

QString QQmlTableModel::columnPropertyNameFromRole(int columnIndex, int role) const
{
    QString propertyName;
    if (role == Qt::DisplayRole && mColumnProperties.at(columnIndex).explicitDisplayRoleIndex == -1) {
        // The user is getting or setting data for the display role,
        // but didn't specify any role with the name "display" in this column.
        // So, we give them the implicit display role, aka the first property we find.
        propertyName = mColumnProperties.at(columnIndex).infoForProperties.first().name;
    } else {
        // QML/JS supports utf8.
        propertyName = QString::fromUtf8(mRoleNames.value(role));
    }
    return propertyName;
}

QT_END_NAMESPACE
