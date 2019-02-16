/*
 * types.h -
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

#ifndef tiger_types_h
#define tiger_types_h

#include "symbol.h"

typedef struct sTy *Ty;
typedef struct sTyField *TyField;

struct sTy {
    enum {
        tTyRecord, tTyNil, tTyInt, tTyString, tTyArray,
        tTyName, tTyVoid
    } kind;

    union {
        List/*<TyField>*/ fields; // tTyRecord
        Ty array;
        struct {Symbol sym; Ty ty;} name; // name type, for arrays/records
    } as;
};

struct sTyField {Symbol name; Ty ty;};

Ty Ty_Nil(void);
Ty Ty_Int(void);
Ty Ty_String(void);
Ty Ty_Void(void);

Ty Ty_Record(List tyFields);
Ty Ty_Array(Ty elType);
Ty Ty_Name(Symbol sym, Ty ty);
TyField Ty_Field(Symbol name, Ty ty);

string Ty_GetName(Ty ty);

void Ty_print(Ty t);
void TyList_print(List tyList);

bool Ty_Match(Ty t1, Ty t2);
// If named type, attemps to retrieve actual type. If none found, returns NULL.
Ty Ty_Actual(Ty t);

#endif
