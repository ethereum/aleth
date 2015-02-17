/****************************************************************************
** Meta object code from reading C++ file 'QVariableDefinition.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/QVariableDefinition.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QVariableDefinition.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__QVariableDefinition_t {
    QByteArrayData data[9];
    char stringdata[107];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QVariableDefinition_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QVariableDefinition_t qt_meta_stringdata_dev__mix__QVariableDefinition = {
    {
QT_MOC_LITERAL(0, 0, 29), // "dev::mix::QVariableDefinition"
QT_MOC_LITERAL(1, 30, 11), // "declaration"
QT_MOC_LITERAL(2, 42, 21), // "QVariableDeclaration*"
QT_MOC_LITERAL(3, 64, 0), // ""
QT_MOC_LITERAL(4, 65, 8), // "setValue"
QT_MOC_LITERAL(5, 74, 6), // "_value"
QT_MOC_LITERAL(6, 81, 14), // "setDeclaration"
QT_MOC_LITERAL(7, 96, 4), // "_dec"
QT_MOC_LITERAL(8, 101, 5) // "value"

    },
    "dev::mix::QVariableDefinition\0declaration\0"
    "QVariableDeclaration*\0\0setValue\0_value\0"
    "setDeclaration\0_dec\0value"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QVariableDefinition[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       2,   36, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // methods: name, argc, parameters, tag, flags
       1,    0,   29,    3, 0x02 /* Public */,
       4,    1,   30,    3, 0x02 /* Public */,
       6,    1,   33,    3, 0x02 /* Public */,

 // methods: parameters
    0x80000000 | 2,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, 0x80000000 | 2,    7,

 // properties: name, type, flags
       8, QMetaType::QString, 0x00095401,
       1, 0x80000000 | 2, 0x00095409,

       0        // eod
};

void dev::mix::QVariableDefinition::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        QVariableDefinition *_t = static_cast<QVariableDefinition *>(_o);
        switch (_id) {
        case 0: { QVariableDeclaration* _r = _t->declaration();
            if (_a[0]) *reinterpret_cast< QVariableDeclaration**>(_a[0]) = _r; }  break;
        case 1: _t->setValue((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->setDeclaration((*reinterpret_cast< QVariableDeclaration*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QVariableDeclaration* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QVariableDeclaration* >(); break;
        }
    }

}

const QMetaObject dev::mix::QVariableDefinition::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__QVariableDefinition.data,
      qt_meta_data_dev__mix__QVariableDefinition,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QVariableDefinition::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QVariableDefinition::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QVariableDefinition.stringdata))
        return static_cast<void*>(const_cast< QVariableDefinition*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::QVariableDefinition::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = value(); break;
        case 1: *reinterpret_cast< QVariableDeclaration**>(_v) = declaration(); break;
        default: break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
struct qt_meta_stringdata_dev__mix__QVariableDefinitionList_t {
    QByteArrayData data[1];
    char stringdata[34];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QVariableDefinitionList_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QVariableDefinitionList_t qt_meta_stringdata_dev__mix__QVariableDefinitionList = {
    {
QT_MOC_LITERAL(0, 0, 33) // "dev::mix::QVariableDefinition..."

    },
    "dev::mix::QVariableDefinitionList"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QVariableDefinitionList[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void dev::mix::QVariableDefinitionList::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QVariableDefinitionList::staticMetaObject = {
    { &QAbstractListModel::staticMetaObject, qt_meta_stringdata_dev__mix__QVariableDefinitionList.data,
      qt_meta_data_dev__mix__QVariableDefinitionList,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QVariableDefinitionList::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QVariableDefinitionList::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QVariableDefinitionList.stringdata))
        return static_cast<void*>(const_cast< QVariableDefinitionList*>(this));
    return QAbstractListModel::qt_metacast(_clname);
}

int dev::mix::QVariableDefinitionList::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_dev__mix__QIntType_t {
    QByteArrayData data[1];
    char stringdata[19];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QIntType_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QIntType_t qt_meta_stringdata_dev__mix__QIntType = {
    {
QT_MOC_LITERAL(0, 0, 18) // "dev::mix::QIntType"

    },
    "dev::mix::QIntType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QIntType[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void dev::mix::QIntType::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QIntType::staticMetaObject = {
    { &QVariableDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QIntType.data,
      qt_meta_data_dev__mix__QIntType,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QIntType::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QIntType::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QIntType.stringdata))
        return static_cast<void*>(const_cast< QIntType*>(this));
    return QVariableDefinition::qt_metacast(_clname);
}

int dev::mix::QIntType::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QVariableDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_dev__mix__QRealType_t {
    QByteArrayData data[1];
    char stringdata[20];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QRealType_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QRealType_t qt_meta_stringdata_dev__mix__QRealType = {
    {
QT_MOC_LITERAL(0, 0, 19) // "dev::mix::QRealType"

    },
    "dev::mix::QRealType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QRealType[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void dev::mix::QRealType::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QRealType::staticMetaObject = {
    { &QVariableDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QRealType.data,
      qt_meta_data_dev__mix__QRealType,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QRealType::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QRealType::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QRealType.stringdata))
        return static_cast<void*>(const_cast< QRealType*>(this));
    return QVariableDefinition::qt_metacast(_clname);
}

int dev::mix::QRealType::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QVariableDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_dev__mix__QStringType_t {
    QByteArrayData data[1];
    char stringdata[22];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QStringType_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QStringType_t qt_meta_stringdata_dev__mix__QStringType = {
    {
QT_MOC_LITERAL(0, 0, 21) // "dev::mix::QStringType"

    },
    "dev::mix::QStringType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QStringType[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void dev::mix::QStringType::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QStringType::staticMetaObject = {
    { &QVariableDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QStringType.data,
      qt_meta_data_dev__mix__QStringType,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QStringType::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QStringType::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QStringType.stringdata))
        return static_cast<void*>(const_cast< QStringType*>(this));
    return QVariableDefinition::qt_metacast(_clname);
}

int dev::mix::QStringType::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QVariableDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_dev__mix__QHashType_t {
    QByteArrayData data[1];
    char stringdata[20];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QHashType_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QHashType_t qt_meta_stringdata_dev__mix__QHashType = {
    {
QT_MOC_LITERAL(0, 0, 19) // "dev::mix::QHashType"

    },
    "dev::mix::QHashType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QHashType[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void dev::mix::QHashType::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QHashType::staticMetaObject = {
    { &QVariableDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QHashType.data,
      qt_meta_data_dev__mix__QHashType,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QHashType::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QHashType::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QHashType.stringdata))
        return static_cast<void*>(const_cast< QHashType*>(this));
    return QVariableDefinition::qt_metacast(_clname);
}

int dev::mix::QHashType::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QVariableDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_dev__mix__QBoolType_t {
    QByteArrayData data[1];
    char stringdata[20];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QBoolType_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QBoolType_t qt_meta_stringdata_dev__mix__QBoolType = {
    {
QT_MOC_LITERAL(0, 0, 19) // "dev::mix::QBoolType"

    },
    "dev::mix::QBoolType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QBoolType[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void dev::mix::QBoolType::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QBoolType::staticMetaObject = {
    { &QVariableDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QBoolType.data,
      qt_meta_data_dev__mix__QBoolType,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QBoolType::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QBoolType::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QBoolType.stringdata))
        return static_cast<void*>(const_cast< QBoolType*>(this));
    return QVariableDefinition::qt_metacast(_clname);
}

int dev::mix::QBoolType::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QVariableDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
