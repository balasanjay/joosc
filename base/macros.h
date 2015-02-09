#ifndef BASE_MACROS_H
#define BASE_MACROS_H

#define RESET_ERRNO errno = 0
#define FIELD_PRINT(name) #name << ":" << name << ","

#define IS_CONST_PTR(type, arg) (dynamic_cast<const type*>(arg) != nullptr)
#define IS_CONST_REF(type, arg) (dynamic_cast<const type*>(&(arg)) != nullptr)

#endif
