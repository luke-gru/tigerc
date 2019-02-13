/*
 * types.c -
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"

static struct sTy tyNil = {
    .kind = tTyNil,
    .as = { .fields =  NULL }
};
static struct sTy tyInt = {
    .kind = tTyInt,
    .as = { .fields = NULL }
};
static struct sTy tyString = {
    .kind = tTyString,
    .as = { .fields = NULL }
};
static struct sTy tyVoid = {
    .kind = tTyVoid,
    .as = { .fields = NULL }
};

Ty Ty_Nil(void) { return &tyNil; }

Ty Ty_Int(void) {return &tyInt;}

Ty Ty_String(void) { return &tyString; }

Ty Ty_Void(void) { return &tyVoid; }

Ty Ty_Record(List fields) {
    Ty p = CHECKED_MALLOC(struct sTy);
    p->kind = tTyRecord;
    p->as.fields = fields;
    return p;
}

Ty Ty_Array(Ty ty) {
    Ty p = CHECKED_MALLOC(struct sTy);
    p->kind = tTyArray;
    p->as.array = ty;
    return p;
}

Ty Ty_Name(Symbol sym, Ty ty) {
    Ty p = CHECKED_MALLOC(struct sTy);
    p->kind = tTyName;
    p->as.name.sym = sym;
    p->as.name.ty = ty;
    return p;
}

TyField Ty_Field(Symbol name, Ty ty) {
    TyField p = CHECKED_MALLOC(struct sTyField);
    p->name = name;
    p->ty = ty;
    return p;
}

/* printing functions - used for debugging */
static char str_ty[][12] = {
   "ty_record", "ty_nil", "ty_int", "ty_string",
   "ty_array", "ty_name", "ty_void" };

/* This will infinite loop on mutually recursive types */
void Ty_print(Ty t) {
    if (t == NULL) {
        printf("null");
    } else {
        printf("%s", str_ty[t->kind]);
        if (t->kind == tTyName) {
            printf(", %s", SymName(t->as.name.sym));
        }
    }
}

void TyList_print(List tyList) {
  if (tyList == NULL) {
      printf("null");
  } else {
      printf("TyList( ");
      Ty_print((Ty)tyList->data);
      printf(", ");
      TyList_print(tyList->next);
      printf(")");
  }
}
