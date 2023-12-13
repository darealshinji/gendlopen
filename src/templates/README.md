The following replacements will be performed in the template code:

`_$`: `GDO_` or uppercase version of user-defined prefix

`$`: `gdo_` or lowercase version of user-defined prefix

`gdo::` and `namespace gdo`: namespace will be replaced if a user-defined prefix was given

`GDO_COMMON`: common header data (`common.h`)

`GDO_TYPEDEFS`: typedef code from the input


Any lines containing one or more of the following will be replaced multiple
times with code from the input (used to generate typedefs, prototyes, etc.):

`GDO_RET`: empty if function doesn't return anything (`void`), else `return`

`GDO_TYPE`: function return type

`GDO_SYMBOL`: function symbol name

`GDO_ARGS`: function arguments

`GDO_NOTYPE_ARGS`: function argument names without type

`GDO_OBJ_TYPE`: object type

`GDO_OBJ_SYMBOL`: object symbol name

`@`: if a line ends on `@` the newline will be ignored during tokenizing but it
    will not be removed

