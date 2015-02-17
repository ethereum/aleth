/****************************************************************************
** Meta object code from reading C++ file 'QFunctionDefinition.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/QFunctionDefinition.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QFunctionDefinition.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__QFunctionDefinition_t {
    QByteArrayData data[3];
    char stringdata[90];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QFunctionDefinition_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QFunctionDefinition_t qt_meta_stringdata_dev__mix__QFunctionDefinition = {
    {
QT_MOC_LITERAL(0, 0, 29), // "dev::mix::QFunctionDefinition"
QT_MOC_LITERAL(1, 30, 10), // "parameters"
QT_MOC_LITERAL(2, 41, 48) // "QQmlListProperty<dev::mix::QV..."

    },
    "dev::mix::QFunctionDefinition\0parameters\0"
    "QQmlListProperty<dev::mix::QVariableDeclaration>"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QFunctionDefinition[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       1,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, 0x80000000 | 2, 0x00095009,

       0        // eod
};

void dev::mix::QFunctionDefinition::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::QFunctionDefinition::staticMetaObject = {
    { &QBasicNodeDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QFunctionDefinition.data,
      qt_meta_data_dev__mix__QFunctionDefinition,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QFunctionDefinition::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QFunctionDefinition::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QFunctionDefinition.stringdata))
        return static_cast<void*>(const_cast< QFunctionDefinition*>(this));
    return QBasicNodeDefinition::qt_metacast(_clname);
}

int dev::mix::QFunctionDefinition::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QBasicNodeDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QQmlListProperty<dev::mix::QVariableDeclaration>*>(_v) = parameters(); break;
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
QT_END_MOC_NAMESPACE
