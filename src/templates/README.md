Any instances of `GDO` and `gdo` will be replaced with a user-defined
prefix (converted to uppercase and lowercase).

Any lines containing one or more of the following will be replaced multiple
times with code from the input (used to generate typedefs, prototyes, etc.):

`%%return%%`: empty if function doesn't return anything (`void`), else `return`

`%%type%%`: function return type

`%%symbol%%`: function symbol name

`%%args%%`: function arguments

`%%notype_args%%`: function argument names without type

`%%obj_type%%`: object type

`%%obj_symbol%%`: object symbol name

`@`: if a line ends on `@` the newline will be ignored during tokenizing but it
    will not be removed

All lines between a line `%SKIP_BEGIN%` and a line `%SKIP_END%`
will be commented out if `--skip-parameter-names` was passed.
