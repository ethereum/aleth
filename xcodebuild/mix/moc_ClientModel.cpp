/****************************************************************************
** Meta object code from reading C++ file 'ClientModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/ClientModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__RecordLogEntry_t {
    QByteArrayData data[9];
    char stringdata[100];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__RecordLogEntry_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__RecordLogEntry_t qt_meta_stringdata_dev__mix__RecordLogEntry = {
    {
QT_MOC_LITERAL(0, 0, 24), // "dev::mix::RecordLogEntry"
QT_MOC_LITERAL(1, 25, 11), // "recordIndex"
QT_MOC_LITERAL(2, 37, 16), // "transactionIndex"
QT_MOC_LITERAL(3, 54, 8), // "contract"
QT_MOC_LITERAL(4, 63, 8), // "function"
QT_MOC_LITERAL(5, 72, 5), // "value"
QT_MOC_LITERAL(6, 78, 7), // "address"
QT_MOC_LITERAL(7, 86, 8), // "returned"
QT_MOC_LITERAL(8, 95, 4) // "call"

    },
    "dev::mix::RecordLogEntry\0recordIndex\0"
    "transactionIndex\0contract\0function\0"
    "value\0address\0returned\0call"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__RecordLogEntry[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       8,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, QMetaType::UInt, 0x00095401,
       2, QMetaType::QString, 0x00095401,
       3, QMetaType::QString, 0x00095401,
       4, QMetaType::QString, 0x00095401,
       5, QMetaType::QString, 0x00095401,
       6, QMetaType::QString, 0x00095401,
       7, QMetaType::QString, 0x00095401,
       8, QMetaType::Bool, 0x00095401,

       0        // eod
};

void dev::mix::RecordLogEntry::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::RecordLogEntry::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__RecordLogEntry.data,
      qt_meta_data_dev__mix__RecordLogEntry,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::RecordLogEntry::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::RecordLogEntry::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__RecordLogEntry.stringdata))
        return static_cast<void*>(const_cast< RecordLogEntry*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::RecordLogEntry::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< uint*>(_v) = m_recordIndex; break;
        case 1: *reinterpret_cast< QString*>(_v) = m_transactionIndex; break;
        case 2: *reinterpret_cast< QString*>(_v) = m_contract; break;
        case 3: *reinterpret_cast< QString*>(_v) = m_function; break;
        case 4: *reinterpret_cast< QString*>(_v) = m_value; break;
        case 5: *reinterpret_cast< QString*>(_v) = m_address; break;
        case 6: *reinterpret_cast< QString*>(_v) = m_returned; break;
        case 7: *reinterpret_cast< bool*>(_v) = m_call; break;
        default: break;
        }
        _id -= 8;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 8;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 8;
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
struct qt_meta_stringdata_dev__mix__ClientModel_t {
    QByteArrayData data[32];
    char stringdata[373];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__ClientModel_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__ClientModel_t qt_meta_stringdata_dev__mix__ClientModel = {
    {
QT_MOC_LITERAL(0, 0, 21), // "dev::mix::ClientModel"
QT_MOC_LITERAL(1, 22, 10), // "runStarted"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 11), // "runComplete"
QT_MOC_LITERAL(4, 46, 13), // "miningStarted"
QT_MOC_LITERAL(5, 60, 14), // "miningComplete"
QT_MOC_LITERAL(6, 75, 18), // "miningStateChanged"
QT_MOC_LITERAL(7, 94, 9), // "runFailed"
QT_MOC_LITERAL(8, 104, 8), // "_message"
QT_MOC_LITERAL(9, 113, 22), // "contractAddressChanged"
QT_MOC_LITERAL(10, 136, 8), // "newBlock"
QT_MOC_LITERAL(11, 145, 15), // "runStateChanged"
QT_MOC_LITERAL(12, 161, 14), // "debugDataReady"
QT_MOC_LITERAL(13, 176, 10), // "_debugData"
QT_MOC_LITERAL(14, 187, 11), // "apiResponse"
QT_MOC_LITERAL(15, 199, 9), // "newRecord"
QT_MOC_LITERAL(16, 209, 15), // "RecordLogEntry*"
QT_MOC_LITERAL(17, 225, 2), // "_r"
QT_MOC_LITERAL(18, 228, 12), // "stateCleared"
QT_MOC_LITERAL(19, 241, 15), // "debugDeployment"
QT_MOC_LITERAL(20, 257, 10), // "setupState"
QT_MOC_LITERAL(21, 268, 6), // "_state"
QT_MOC_LITERAL(22, 275, 11), // "debugRecord"
QT_MOC_LITERAL(23, 287, 6), // "_index"
QT_MOC_LITERAL(24, 294, 12), // "showDebugger"
QT_MOC_LITERAL(25, 307, 14), // "showDebugError"
QT_MOC_LITERAL(26, 322, 6), // "_error"
QT_MOC_LITERAL(27, 329, 7), // "apiCall"
QT_MOC_LITERAL(28, 337, 4), // "mine"
QT_MOC_LITERAL(29, 342, 7), // "running"
QT_MOC_LITERAL(30, 350, 6), // "mining"
QT_MOC_LITERAL(31, 357, 15) // "contractAddress"

    },
    "dev::mix::ClientModel\0runStarted\0\0"
    "runComplete\0miningStarted\0miningComplete\0"
    "miningStateChanged\0runFailed\0_message\0"
    "contractAddressChanged\0newBlock\0"
    "runStateChanged\0debugDataReady\0"
    "_debugData\0apiResponse\0newRecord\0"
    "RecordLogEntry*\0_r\0stateCleared\0"
    "debugDeployment\0setupState\0_state\0"
    "debugRecord\0_index\0showDebugger\0"
    "showDebugError\0_error\0apiCall\0mine\0"
    "running\0mining\0contractAddress"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__ClientModel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       3,  150, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  114,    2, 0x06 /* Public */,
       3,    0,  115,    2, 0x06 /* Public */,
       4,    0,  116,    2, 0x06 /* Public */,
       5,    0,  117,    2, 0x06 /* Public */,
       6,    0,  118,    2, 0x06 /* Public */,
       7,    1,  119,    2, 0x06 /* Public */,
       9,    0,  122,    2, 0x06 /* Public */,
      10,    0,  123,    2, 0x06 /* Public */,
      11,    0,  124,    2, 0x06 /* Public */,
      12,    1,  125,    2, 0x06 /* Public */,
      14,    1,  128,    2, 0x06 /* Public */,
      15,    1,  131,    2, 0x06 /* Public */,
      18,    0,  134,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      19,    0,  135,    2, 0x0a /* Public */,
      20,    1,  136,    2, 0x0a /* Public */,
      22,    1,  139,    2, 0x0a /* Public */,
      24,    0,  142,    2, 0x08 /* Private */,
      25,    1,  143,    2, 0x08 /* Private */,

 // methods: name, argc, parameters, tag, flags
      27,    1,  146,    2, 0x02 /* Public */,
      28,    0,  149,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QObjectStar,   13,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, 0x80000000 | 16,   17,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QVariantMap,   21,
    QMetaType::Void, QMetaType::UInt,   23,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   26,

 // methods: parameters
    QMetaType::QString, QMetaType::QString,    8,
    QMetaType::Void,

 // properties: name, type, flags
      29, QMetaType::Bool, 0x00495003,
      30, QMetaType::Bool, 0x00495003,
      31, QMetaType::QString, 0x00495001,

 // properties: notify_signal_id
       8,
       4,
       6,

       0        // eod
};

void dev::mix::ClientModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ClientModel *_t = static_cast<ClientModel *>(_o);
        switch (_id) {
        case 0: _t->runStarted(); break;
        case 1: _t->runComplete(); break;
        case 2: _t->miningStarted(); break;
        case 3: _t->miningComplete(); break;
        case 4: _t->miningStateChanged(); break;
        case 5: _t->runFailed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->contractAddressChanged(); break;
        case 7: _t->newBlock(); break;
        case 8: _t->runStateChanged(); break;
        case 9: _t->debugDataReady((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        case 10: _t->apiResponse((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->newRecord((*reinterpret_cast< RecordLogEntry*(*)>(_a[1]))); break;
        case 12: _t->stateCleared(); break;
        case 13: _t->debugDeployment(); break;
        case 14: _t->setupState((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 15: _t->debugRecord((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 16: _t->showDebugger(); break;
        case 17: _t->showDebugError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: { QString _r = _t->apiCall((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 19: _t->mine(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< RecordLogEntry* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::runStarted)) {
                *result = 0;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::runComplete)) {
                *result = 1;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::miningStarted)) {
                *result = 2;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::miningComplete)) {
                *result = 3;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::miningStateChanged)) {
                *result = 4;
            }
        }
        {
            typedef void (ClientModel::*_t)(QString const & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::runFailed)) {
                *result = 5;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::contractAddressChanged)) {
                *result = 6;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::newBlock)) {
                *result = 7;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::runStateChanged)) {
                *result = 8;
            }
        }
        {
            typedef void (ClientModel::*_t)(QObject * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::debugDataReady)) {
                *result = 9;
            }
        }
        {
            typedef void (ClientModel::*_t)(QString const & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::apiResponse)) {
                *result = 10;
            }
        }
        {
            typedef void (ClientModel::*_t)(RecordLogEntry * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::newRecord)) {
                *result = 11;
            }
        }
        {
            typedef void (ClientModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ClientModel::stateCleared)) {
                *result = 12;
            }
        }
    }
}

const QMetaObject dev::mix::ClientModel::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__ClientModel.data,
      qt_meta_data_dev__mix__ClientModel,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::ClientModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::ClientModel::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__ClientModel.stringdata))
        return static_cast<void*>(const_cast< ClientModel*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::ClientModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = m_running; break;
        case 1: *reinterpret_cast< bool*>(_v) = m_mining; break;
        case 2: *reinterpret_cast< QString*>(_v) = contractAddress(); break;
        default: break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0:
            if (m_running != *reinterpret_cast< bool*>(_v)) {
                m_running = *reinterpret_cast< bool*>(_v);
                Q_EMIT runStateChanged();
            }
            break;
        case 1:
            if (m_mining != *reinterpret_cast< bool*>(_v)) {
                m_mining = *reinterpret_cast< bool*>(_v);
                Q_EMIT miningStateChanged();
            }
            break;
        default: break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void dev::mix::ClientModel::runStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void dev::mix::ClientModel::runComplete()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void dev::mix::ClientModel::miningStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}

// SIGNAL 3
void dev::mix::ClientModel::miningComplete()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}

// SIGNAL 4
void dev::mix::ClientModel::miningStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, Q_NULLPTR);
}

// SIGNAL 5
void dev::mix::ClientModel::runFailed(QString const & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void dev::mix::ClientModel::contractAddressChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, Q_NULLPTR);
}

// SIGNAL 7
void dev::mix::ClientModel::newBlock()
{
    QMetaObject::activate(this, &staticMetaObject, 7, Q_NULLPTR);
}

// SIGNAL 8
void dev::mix::ClientModel::runStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, Q_NULLPTR);
}

// SIGNAL 9
void dev::mix::ClientModel::debugDataReady(QObject * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void dev::mix::ClientModel::apiResponse(QString const & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void dev::mix::ClientModel::newRecord(RecordLogEntry * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void dev::mix::ClientModel::stateCleared()
{
    QMetaObject::activate(this, &staticMetaObject, 12, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
