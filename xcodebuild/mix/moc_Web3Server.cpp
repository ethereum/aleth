/****************************************************************************
** Meta object code from reading C++ file 'Web3Server.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/Web3Server.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Web3Server.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__Web3Server_t {
    QByteArrayData data[3];
    char stringdata[37];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__Web3Server_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__Web3Server_t qt_meta_stringdata_dev__mix__Web3Server = {
    {
QT_MOC_LITERAL(0, 0, 20), // "dev::mix::Web3Server"
QT_MOC_LITERAL(1, 21, 14), // "newTransaction"
QT_MOC_LITERAL(2, 36, 0) // ""

    },
    "dev::mix::Web3Server\0newTransaction\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__Web3Server[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

void dev::mix::Web3Server::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Web3Server *_t = static_cast<Web3Server *>(_o);
        switch (_id) {
        case 0: _t->newTransaction(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (Web3Server::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&Web3Server::newTransaction)) {
                *result = 0;
            }
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::Web3Server::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__Web3Server.data,
      qt_meta_data_dev__mix__Web3Server,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::Web3Server::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::Web3Server::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__Web3Server.stringdata))
        return static_cast<void*>(const_cast< Web3Server*>(this));
    if (!strcmp(_clname, "dev::WebThreeStubServerBase"))
        return static_cast< dev::WebThreeStubServerBase*>(const_cast< Web3Server*>(this));
    if (!strcmp(_clname, "dev::WebThreeStubDatabaseFace"))
        return static_cast< dev::WebThreeStubDatabaseFace*>(const_cast< Web3Server*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::Web3Server::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void dev::mix::Web3Server::newTransaction()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
