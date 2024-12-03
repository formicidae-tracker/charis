#pragma once

#if __has_include(<magic_enum/magic_enum.hpp>)
#define CHARIS_OPTIONS_USE_MAGIC_ENUM 1
#endif

#if __has_include(<fkYAML/node.hpp>)
#define CHARIS_OPTIONS_USE_FKYAML
#endif
