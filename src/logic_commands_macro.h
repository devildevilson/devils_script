#ifndef DEVILS_ENGINE_SCRIPT_LOGIC_COMMANDS_MACRO_H
#define DEVILS_ENGINE_SCRIPT_LOGIC_COMMANDS_MACRO_H

// почему тут названия с большой буквы? потому что часть этих названий присутствует в качестве ключевых слов луа

#define LOGIC_BLOCK_COMMANDS_LIST        \
  LOGIC_BLOCK_COMMAND_FUNC(AND)          \
  LOGIC_BLOCK_COMMAND_FUNC(OR)           \
  LOGIC_BLOCK_COMMAND_FUNC(NAND)         \
  LOGIC_BLOCK_COMMAND_FUNC(NOR)          \
  LOGIC_BLOCK_COMMAND_FUNC(XOR)          \
  LOGIC_BLOCK_COMMAND_FUNC(IMPL)         \
  LOGIC_BLOCK_COMMAND_FUNC(EQ)           \
  LOGIC_BLOCK_COMMAND_FUNC(AND_sequence) \
  LOGIC_BLOCK_COMMAND_FUNC(OR_sequence)  \

#endif
