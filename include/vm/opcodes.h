#pragma once
#include <cstdint>
namespace madronavm {
// Using an enum class for type safety. The underlying type is uint32_t.
enum class OpCode : uint32_t {
    LOAD_K  = 0x01, // Operands: dest_reg, value (float bit-cast to u32)
    PROC    = 0x02, // Operands: module_id, num_inputs, num_outputs, in_regs..., out_regs...
    END     = 0xFF, // No operands
};
// The magic number for identifying Madrona VM bytecode files. 'MADR' in ASCII.
const uint32_t kMagicNumber = 0x4D414452;
const uint32_t kBytecodeVersion = 1;
// The header at the beginning of every bytecode buffer.
struct BytecodeHeader {
    uint32_t magic_number;
    uint32_t version;
    uint32_t program_size_words; // Total size of bytecode, including header.
    uint32_t num_registers;      // Number of DSPVector registers required.
};
} // namespace madronavm 