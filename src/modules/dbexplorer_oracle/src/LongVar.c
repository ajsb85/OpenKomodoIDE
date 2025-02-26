//-----------------------------------------------------------------------------
// LongVar.c
//   Defines the routines specific to the long type.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// long type
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    char *data;
} udt_LongVar;


//-----------------------------------------------------------------------------
// declaration of long variable functions.
//-----------------------------------------------------------------------------
static int LongVar_SetValue(udt_LongVar*, unsigned, PyObject*);
static PyObject *LongVar_GetValue(udt_LongVar*, unsigned);
static ub4 LongVar_GetBufferSize(udt_LongVar*);


//-----------------------------------------------------------------------------
// Python type declarations
//-----------------------------------------------------------------------------
static PyTypeObject g_LongStringVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cx_Oracle.LONG_STRING",            // tp_name
    sizeof(udt_LongVar),                // tp_basicsize
    0,                                  // tp_itemsize
    0,                                  // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    0                                   // tp_doc
};


static PyTypeObject g_LongBinaryVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cx_Oracle.LONG_BINARY",            // tp_name
    sizeof(udt_LongVar),                // tp_basicsize
    0,                                  // tp_itemsize
    0,                                  // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    0                                   // tp_doc
};


//-----------------------------------------------------------------------------
// variable type declarations
//-----------------------------------------------------------------------------
static udt_VariableType vt_LongString = {
    (InitializeProc) NULL,
    (FinalizeProc) NULL,
    (PreDefineProc) NULL,
    (PostDefineProc) NULL,
    (IsNullProc) NULL,
    (SetValueProc) LongVar_SetValue,
    (GetValueProc) LongVar_GetValue,
    (GetBufferSizeProc) LongVar_GetBufferSize,
    &g_LongStringVarType,               // Python type
    SQLT_LVC,                           // Oracle type
    SQLCS_IMPLICIT,                     // charset form
    128 * 1024,                         // element length (default)
    1,                                  // is character data
    1,                                  // is variable length
    1,                                  // can be copied
    0                                   // can be in array
};


static udt_VariableType vt_LongBinary = {
    (InitializeProc) NULL,
    (FinalizeProc) NULL,
    (PreDefineProc) NULL,
    (PostDefineProc) NULL,
    (IsNullProc) NULL,
    (SetValueProc) LongVar_SetValue,
    (GetValueProc) LongVar_GetValue,
    (GetBufferSizeProc) LongVar_GetBufferSize,
    &g_LongBinaryVarType,               // Python type
    SQLT_LVB,                           // Oracle type
    SQLCS_IMPLICIT,                     // charset form
    128 * 1024,                         // element length (default)
    0,                                  // is character data
    1,                                  // is variable length
    1,                                  // can be copied
    0                                   // can be in array
};


//-----------------------------------------------------------------------------
// LongVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int LongVar_SetValue(
    udt_LongVar *var,                   // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    udt_StringBuffer buffer;
    char *ptr;
    ub4 size;

    // get the buffer data and size for binding
    if (var->type == &vt_LongBinary) {
        if (PyBytes_Check(value)) {
            if (StringBuffer_FromBytes(&buffer, value) < 0)
                return -1;
#if PY_MAJOR_VERSION < 3
        } else if (cxBinary_Check(value)) {
            if (StringBuffer_FromBinary(&buffer, value) < 0)
                return -1;
#endif
        } else {
            PyErr_SetString(PyExc_TypeError,
                    "expecting string or buffer data");
            return -1;
        }
        size = buffer.size;
    } else {
        if (!cxString_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "expecting string data");
            return -1;
        }
        if (StringBuffer_Fill(&buffer, value) < 0)
            return -1;
        size = cxString_GetSize(value);
    }

    // verify there is enough space to store the value
    if (size > var->size) {
        if (Variable_Resize((udt_Variable*) var, size) < 0) {
            StringBuffer_Clear(&buffer);
            return -1;
        }
    }

    // copy the string to the Oracle buffer
    ptr = var->data + var->bufferSize * pos;
    *((ub4 *) ptr) = (ub4) buffer.size;
    if (buffer.size)
        memcpy(ptr + sizeof(ub4), buffer.ptr, buffer.size);
    StringBuffer_Clear(&buffer);

    return 0;
}


//-----------------------------------------------------------------------------
// LongVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *LongVar_GetValue(
    udt_LongVar *var,                   // variable to determine value for
    unsigned pos)                       // array position
{
    char *ptr;
    ub4 size;

    ptr = var->data + var->bufferSize * pos;
    size = *((ub4 *) ptr);
    ptr += sizeof(ub4);
    if (var->type == &vt_LongBinary)
        return PyBytes_FromStringAndSize(ptr, size);
    return cxString_FromEncodedString(ptr, size);
}


//-----------------------------------------------------------------------------
// LongVar_GetBufferSize()
//   Returns the size of the buffer to use for data of the given size.
//-----------------------------------------------------------------------------
static ub4 LongVar_GetBufferSize(
    udt_LongVar *self)                  // variable to get buffer size
{
    if (!self->type->isCharacterData)
        return self->size + sizeof(ub4);
#ifdef WITH_UNICODE
    return sizeof(ub4) + self->size * CXORA_BYTES_PER_CHAR;
#else
    return sizeof(ub4) + self->size * self->environment->maxBytesPerCharacter;
#endif
}

