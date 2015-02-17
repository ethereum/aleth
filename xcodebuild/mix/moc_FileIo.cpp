/****************************************************************************
** Meta object code from reading C++ file 'FileIo.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/FileIo.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FileIo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__FileIo_t {
    QByteArrayData data[15];
    char stringdata[131];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__FileIo_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__FileIo_t qt_meta_stringdata_dev__mix__FileIo = {
    {
QT_MOC_LITERAL(0, 0, 16), // "dev::mix::FileIo"
QT_MOC_LITERAL(1, 17, 5), // "error"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 10), // "_errorText"
QT_MOC_LITERAL(4, 35, 7), // "makeDir"
QT_MOC_LITERAL(5, 43, 4), // "_url"
QT_MOC_LITERAL(6, 48, 8), // "readFile"
QT_MOC_LITERAL(7, 57, 9), // "writeFile"
QT_MOC_LITERAL(8, 67, 5), // "_data"
QT_MOC_LITERAL(9, 73, 8), // "copyFile"
QT_MOC_LITERAL(10, 82, 10), // "_sourceUrl"
QT_MOC_LITERAL(11, 93, 8), // "_destUrl"
QT_MOC_LITERAL(12, 102, 8), // "moveFile"
QT_MOC_LITERAL(13, 111, 10), // "fileExists"
QT_MOC_LITERAL(14, 122, 8) // "homePath"

    },
    "dev::mix::FileIo\0error\0\0_errorText\0"
    "makeDir\0_url\0readFile\0writeFile\0_data\0"
    "copyFile\0_sourceUrl\0_destUrl\0moveFile\0"
    "fileExists\0homePath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__FileIo[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       1,   76, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       4,    1,   52,    2, 0x02 /* Public */,
       6,    1,   55,    2, 0x02 /* Public */,
       7,    2,   58,    2, 0x02 /* Public */,
       9,    2,   63,    2, 0x02 /* Public */,
      12,    2,   68,    2, 0x02 /* Public */,
      13,    1,   73,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // methods: parameters
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::QString, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    5,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   11,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   11,
    QMetaType::Bool, QMetaType::QString,    5,

 // properties: name, type, flags
      14, QMetaType::QString, 0x00095401,

       0        // eod
};

void dev::mix::FileIo::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FileIo *_t = static_cast<FileIo *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->makeDir((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: { QString _r = _t->readFile((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 3: _t->writeFile((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->copyFile((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 5: _t->moveFile((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 6: { bool _r = _t->fileExists((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FileIo::*_t)(QString const & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FileIo::error)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject dev::mix::FileIo::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__FileIo.data,
      qt_meta_data_dev__mix__FileIo,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::FileIo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::FileIo::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__FileIo.stringdata))
        return static_cast<void*>(const_cast< FileIo*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::FileIo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = getHomePath(); break;
        default: break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void dev::mix::FileIo::error(QString const & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
