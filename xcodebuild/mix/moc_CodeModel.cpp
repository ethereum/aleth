/****************************************************************************
** Meta object code from reading C++ file 'CodeModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../mix/CodeModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CodeModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dev__mix__BackgroundWorker_t {
    QByteArrayData data[5];
    char stringdata[60];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__BackgroundWorker_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__BackgroundWorker_t qt_meta_stringdata_dev__mix__BackgroundWorker = {
    {
QT_MOC_LITERAL(0, 0, 26), // "dev::mix::BackgroundWorker"
QT_MOC_LITERAL(1, 27, 15), // "queueCodeChange"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 6), // "_jobId"
QT_MOC_LITERAL(4, 51, 8) // "_content"

    },
    "dev::mix::BackgroundWorker\0queueCodeChange\0"
    "\0_jobId\0_content"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__BackgroundWorker[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   19,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    4,

       0        // eod
};

void dev::mix::BackgroundWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        BackgroundWorker *_t = static_cast<BackgroundWorker *>(_o);
        switch (_id) {
        case 0: _t->queueCodeChange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject dev::mix::BackgroundWorker::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__BackgroundWorker.data,
      qt_meta_data_dev__mix__BackgroundWorker,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::BackgroundWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::BackgroundWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__BackgroundWorker.stringdata))
        return static_cast<void*>(const_cast< BackgroundWorker*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::BackgroundWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
struct qt_meta_stringdata_dev__mix__CompilationResult_t {
    QByteArrayData data[7];
    char stringdata[111];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__CompilationResult_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__CompilationResult_t qt_meta_stringdata_dev__mix__CompilationResult = {
    {
QT_MOC_LITERAL(0, 0, 27), // "dev::mix::CompilationResult"
QT_MOC_LITERAL(1, 28, 8), // "contract"
QT_MOC_LITERAL(2, 37, 20), // "QContractDefinition*"
QT_MOC_LITERAL(3, 58, 15), // "compilerMessage"
QT_MOC_LITERAL(4, 74, 10), // "successful"
QT_MOC_LITERAL(5, 85, 17), // "contractInterface"
QT_MOC_LITERAL(6, 103, 7) // "codeHex"

    },
    "dev::mix::CompilationResult\0contract\0"
    "QContractDefinition*\0compilerMessage\0"
    "successful\0contractInterface\0codeHex"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__CompilationResult[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       5,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, 0x80000000 | 2, 0x00095009,
       3, QMetaType::QString, 0x00095401,
       4, QMetaType::Bool, 0x00095401,
       5, QMetaType::QString, 0x00095401,
       6, QMetaType::QString, 0x00095401,

       0        // eod
};

void dev::mix::CompilationResult::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject dev::mix::CompilationResult::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__CompilationResult.data,
      qt_meta_data_dev__mix__CompilationResult,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::CompilationResult::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::CompilationResult::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__CompilationResult.stringdata))
        return static_cast<void*>(const_cast< CompilationResult*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::CompilationResult::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QContractDefinition**>(_v) = contract(); break;
        case 1: *reinterpret_cast< QString*>(_v) = compilerMessage(); break;
        case 2: *reinterpret_cast< bool*>(_v) = successful(); break;
        case 3: *reinterpret_cast< QString*>(_v) = contractInterface(); break;
        case 4: *reinterpret_cast< QString*>(_v) = codeHex(); break;
        default: break;
        }
        _id -= 5;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 5;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 5;
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
struct qt_meta_stringdata_dev__mix__CodeModel_t {
    QByteArrayData data[18];
    char stringdata[262];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dev__mix__CodeModel_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dev__mix__CodeModel_t qt_meta_stringdata_dev__mix__CodeModel = {
    {
QT_MOC_LITERAL(0, 0, 19), // "dev::mix::CodeModel"
QT_MOC_LITERAL(1, 20, 12), // "stateChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 19), // "compilationComplete"
QT_MOC_LITERAL(4, 54, 22), // "scheduleCompilationJob"
QT_MOC_LITERAL(5, 77, 6), // "_jobId"
QT_MOC_LITERAL(6, 84, 8), // "_content"
QT_MOC_LITERAL(7, 93, 11), // "codeChanged"
QT_MOC_LITERAL(8, 105, 24), // "contractInterfaceChanged"
QT_MOC_LITERAL(9, 130, 27), // "compilationCompleteInternal"
QT_MOC_LITERAL(10, 158, 18), // "CompilationResult*"
QT_MOC_LITERAL(11, 177, 10), // "_newResult"
QT_MOC_LITERAL(12, 188, 21), // "onCompilationComplete"
QT_MOC_LITERAL(13, 210, 18), // "registerCodeChange"
QT_MOC_LITERAL(14, 229, 5), // "_code"
QT_MOC_LITERAL(15, 235, 4), // "code"
QT_MOC_LITERAL(16, 240, 9), // "compiling"
QT_MOC_LITERAL(17, 250, 11) // "hasContract"

    },
    "dev::mix::CodeModel\0stateChanged\0\0"
    "compilationComplete\0scheduleCompilationJob\0"
    "_jobId\0_content\0codeChanged\0"
    "contractInterfaceChanged\0"
    "compilationCompleteInternal\0"
    "CompilationResult*\0_newResult\0"
    "onCompilationComplete\0registerCodeChange\0"
    "_code\0code\0compiling\0hasContract"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dev__mix__CodeModel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       3,   72, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    2,   56,    2, 0x06 /* Public */,
       7,    0,   61,    2, 0x06 /* Public */,
       8,    0,   62,    2, 0x06 /* Public */,
       9,    1,   63,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,   66,    2, 0x08 /* Private */,
      13,    1,   69,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    5,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10,   11,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, QMetaType::QString,   14,

 // properties: name, type, flags
      15, 0x80000000 | 10, 0x00495009,
      16, QMetaType::Bool, 0x00495001,
      17, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       3,
       0,
       3,

       0        // eod
};

void dev::mix::CodeModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CodeModel *_t = static_cast<CodeModel *>(_o);
        switch (_id) {
        case 0: _t->stateChanged(); break;
        case 1: _t->compilationComplete(); break;
        case 2: _t->scheduleCompilationJob((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->codeChanged(); break;
        case 4: _t->contractInterfaceChanged(); break;
        case 5: _t->compilationCompleteInternal((*reinterpret_cast< CompilationResult*(*)>(_a[1]))); break;
        case 6: _t->onCompilationComplete((*reinterpret_cast< CompilationResult*(*)>(_a[1]))); break;
        case 7: _t->registerCodeChange((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CompilationResult* >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CompilationResult* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (CodeModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CodeModel::stateChanged)) {
                *result = 0;
            }
        }
        {
            typedef void (CodeModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CodeModel::compilationComplete)) {
                *result = 1;
            }
        }
        {
            typedef void (CodeModel::*_t)(int , QString const & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CodeModel::scheduleCompilationJob)) {
                *result = 2;
            }
        }
        {
            typedef void (CodeModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CodeModel::codeChanged)) {
                *result = 3;
            }
        }
        {
            typedef void (CodeModel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CodeModel::contractInterfaceChanged)) {
                *result = 4;
            }
        }
        {
            typedef void (CodeModel::*_t)(CompilationResult * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CodeModel::compilationCompleteInternal)) {
                *result = 5;
            }
        }
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CompilationResult* >(); break;
        }
    }

}

const QMetaObject dev::mix::CodeModel::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_dev__mix__CodeModel.data,
      qt_meta_data_dev__mix__CodeModel,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dev::mix::CodeModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dev::mix::CodeModel::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dev__mix__CodeModel.stringdata))
        return static_cast<void*>(const_cast< CodeModel*>(this));
    return QObject::qt_metacast(_clname);
}

int dev::mix::CodeModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< CompilationResult**>(_v) = code(); break;
        case 1: *reinterpret_cast< bool*>(_v) = isCompiling(); break;
        case 2: *reinterpret_cast< bool*>(_v) = hasContract(); break;
        default: break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void dev::mix::CodeModel::stateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void dev::mix::CodeModel::compilationComplete()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void dev::mix::CodeModel::scheduleCompilationJob(int _t1, QString const & _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void dev::mix::CodeModel::codeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}

// SIGNAL 4
void dev::mix::CodeModel::contractInterfaceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, Q_NULLPTR);
}

// SIGNAL 5
void dev::mix::CodeModel::compilationCompleteInternal(CompilationResult * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
