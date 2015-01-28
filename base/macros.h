#ifndef BASE_MACROS_H
#define BASE_MACROS_H

#define RESET_ERRNO errno = 0

#define IS_CONST_PTR(type, arg) (dynamic_cast<const type*>(arg) != nullptr)

#endif
