/****************************************************************************
** Meta object code from reading C++ file 'QContractDefinition.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/QContractDefinition.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QContractDefinition.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__QContractDefinition_t {
    QByteArrayData data[5];
    char stringdata[131];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__QContractDefinition_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__QContractDefinition_t qt_meta_stringdata_dev__mix__QContractDefinition = {
    {
QT_MOC_LITERAL(0, 0, 29), // "dev::mix::QContractDefinition"
QT_MOC_LITERAL(1, 30, 9), // "functions"
QT_MOC_LITERAL(2, 40, 47), // "QQmlListProperty<dev::mix::QF..."
QT_MOC_LITERAL(3, 88, 11), // "constructor"
QT_MOC_LITERAL(4, 100, 30) // "dev::mix::QFunctionDefinition*"

    },
    "dev::mix::QContractDefinition\0functions\0"
    "QQmlListProperty<dev::mix::QFunctionDefinition>\0"
    "constructor\0dev::mix::QFunctionDefinition*"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__QContractDefinition[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       2,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, 0x80000000 | 2, 0x00095409,
       3, 0x80000000 | 4, 0x00095409,

       0        // eod
};

void dev::mix::QContractDefinition::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< dev::mix::QFunctionDefinition* >(); break;
        }
    }

    Q_UNUSED(_o);
}

const QMetaObject dev::mix::QContractDefinition::staticMetaObject = {
    { &QBasicNodeDefinition::staticMetaObject, qt_meta_stringdata_dev__mix__QContractDefinition.data,
      qt_meta_data_dev__mix__QContractDefinition,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::QContractDefinition::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::QContractDefinition::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__QContractDefinition.stringdata))
        return static_cast<void*>(const_cast< QContractDefinition*>(this));
    return QBasicNodeDefinition::qt_metacast(_clname);
}

int dev::mix::QContractDefinition::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QBasicNodeDefinition::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QQmlListProperty<dev::mix::QFunctionDefinition>*>(_v) = functions(); break;
        case 1: *reinterpret_cast< dev::mix::QFunctionDefinition**>(_v) = constructor(); break;
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
QT_END_MOC_NAMESPACE
