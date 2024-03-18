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

All lines between a line `%SKIP_BEGIN%\n` and a line `%SKIP_END%\n`
will be commented out if `--skip-parameter-names` was passed.


Any remaining instances of `GDO` and `gdo` will be replaced with a user-defined
prefix (converted to uppercase and lowercase).
