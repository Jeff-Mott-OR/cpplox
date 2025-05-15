#include "chunk.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/endian/conversion.hpp>
#include <gsl/gsl>

#include "object.hpp"

namespace motts::lox
{
    std::ostream& operator<<(std::ostream& os, Opcode opcode)
    {
        const char* opcode_name = [&] {
            switch (opcode) {
                default: {
                    throw std::logic_error{"Unexpected opcode."};
                }

#define X(name) \
    case Opcode::name: \
        return #name;
                    MOTTS_LOX_OPCODE_NAMES
#undef X
            }
        }();

        // Names should print as uppercase without trailing underscores.
        std::string name_str{opcode_name};
        boost::trim_right_if(name_str, boost::is_any_of("_"));
        boost::to_upper(name_str);

        os << name_str;

        return os;
    }

    Chunk::Jump_backpatch::Jump_backpatch(std::vector<std::uint8_t>& bytecode)
        : bytecode_{bytecode},
          jump_begin_index_{bytecode.size()}
    {
    }

    void Chunk::Jump_backpatch::to_next_opcode()
    {
        const auto jump_distance = gsl::narrow<std::uint16_t>(bytecode_.size() - jump_begin_index_);
        const auto jump_distance_big_endian = boost::endian::native_to_big(jump_distance);
        reinterpret_cast<std::uint16_t&>(bytecode_.at(jump_begin_index_ - 2)) = jump_distance_big_endian;
    }

    std::size_t Chunk::insert_constant(Dynamic_type_value value)
    {
        const auto maybe_duplicate_iter = std::find(constants_.cbegin(), constants_.cend(), value);
        if (maybe_duplicate_iter != constants_.cend()) {
            const auto constant_index = maybe_duplicate_iter - constants_.cbegin();
            return gsl::narrow<std::size_t>(constant_index);
        }

        const auto constant_index = constants_.size();
        constants_.push_back(value);

        return constant_index;
    }

    void Chunk::emit(std::uint8_t byte, const Source_map_token& token)
    {
        bytecode_.push_back(byte);
        source_map_tokens_.push_back(token);
    }

    template<Opcode opcode>
    void Chunk::emit(const Source_map_token& token)
    {
        emit(gsl::narrow<std::uint8_t>(opcode), token);
    }

    // Explicit instantiation of the opcodes allowed with this templated member function.
    template void Chunk::emit<Opcode::add>(const Source_map_token&);
    template void Chunk::emit<Opcode::close_upvalue>(const Source_map_token&);
    template void Chunk::emit<Opcode::divide>(const Source_map_token&);
    template void Chunk::emit<Opcode::equal>(const Source_map_token&);
    template void Chunk::emit<Opcode::false_>(const Source_map_token&);
    template void Chunk::emit<Opcode::greater>(const Source_map_token&);
    template void Chunk::emit<Opcode::inherit>(const Source_map_token&);
    template void Chunk::emit<Opcode::less>(const Source_map_token&);
    template void Chunk::emit<Opcode::multiply>(const Source_map_token&);
    template void Chunk::emit<Opcode::negate>(const Source_map_token&);
    template void Chunk::emit<Opcode::nil>(const Source_map_token&);
    template void Chunk::emit<Opcode::not_>(const Source_map_token&);
    template void Chunk::emit<Opcode::pop>(const Source_map_token&);
    template void Chunk::emit<Opcode::print>(const Source_map_token&);
    template void Chunk::emit<Opcode::return_>(const Source_map_token&);
    template void Chunk::emit<Opcode::subtract>(const Source_map_token&);
    template void Chunk::emit<Opcode::true_>(const Source_map_token&);

    template<Opcode opcode>
    void Chunk::emit(GC_ptr<const std::string> identifier_name, const Source_map_token& token)
    {
        const auto constant_index = insert_constant(identifier_name);

        emit(gsl::narrow<std::uint8_t>(opcode), token);
        emit(gsl::narrow<std::uint8_t>(constant_index), token);
    }

    template void Chunk::emit<Opcode::class_>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::define_global>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::get_global>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::get_property>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::get_super>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::method>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::set_global>(GC_ptr<const std::string>, const Source_map_token&);
    template void Chunk::emit<Opcode::set_property>(GC_ptr<const std::string>, const Source_map_token&);

    template<Opcode opcode>
    void Chunk::emit(unsigned int index, const Source_map_token& token)
    {
        emit(gsl::narrow<std::uint8_t>(opcode), token);
        emit(gsl::narrow<std::uint8_t>(index), token);
    }

    template void Chunk::emit<Opcode::get_local>(unsigned int, const Source_map_token&);
    template void Chunk::emit<Opcode::get_upvalue>(unsigned int, const Source_map_token&);
    template void Chunk::emit<Opcode::set_local>(unsigned int, const Source_map_token&);
    template void Chunk::emit<Opcode::set_upvalue>(unsigned int, const Source_map_token&);

    void Chunk::emit_call(unsigned int arg_count, const Source_map_token& token)
    {
        emit(gsl::narrow<std::uint8_t>(Opcode::call), token);
        emit(gsl::narrow<std::uint8_t>(arg_count), token);
    }

    void Chunk::emit_closure(GC_ptr<Function> fn, const std::vector<Tracked_upvalue>& tracked_upvalues, const Source_map_token& token)
    {
        const auto fn_constant_index = insert_constant(fn);

        emit(gsl::narrow<std::uint8_t>(Opcode::closure), token);
        emit(gsl::narrow<std::uint8_t>(fn_constant_index), token);
        emit(gsl::narrow<std::uint8_t>(tracked_upvalues.size()), token);

        for (const auto& tracked_upvalue : tracked_upvalues) {
            // To match clox opcodes (which isn't necessarily important to do),
            // a `1` means parent local, and a `0` means parent upvalue.
            if (const auto* upvalue = std::get_if<Upvalue_index>(&tracked_upvalue)) {
                emit(1, token);
                emit(gsl::narrow<std::uint8_t>(upvalue->enclosing_locals_index), token);
            } else {
                const auto& upupvalue = std::get<UpUpvalue_index>(tracked_upvalue);
                emit(0, token);
                emit(gsl::narrow<std::uint8_t>(upupvalue.enclosing_upvalues_index), token);
            }
        }
    }

    void Chunk::emit_constant(Dynamic_type_value value, const Source_map_token& token)
    {
        const auto constant_index = insert_constant(value);

        emit(gsl::narrow<std::uint8_t>(Opcode::constant), token);
        emit(gsl::narrow<std::uint8_t>(constant_index), token);
    }

    Chunk::Jump_backpatch Chunk::emit_jump(const Source_map_token& token)
    {
        emit(gsl::narrow<std::uint8_t>(Opcode::jump), token);
        emit(0, token);
        emit(0, token);

        return Jump_backpatch{bytecode_};
    }

    Chunk::Jump_backpatch Chunk::emit_jump_if_false(const Source_map_token& token)
    {
        emit(gsl::narrow<std::uint8_t>(Opcode::jump_if_false), token);
        emit(0, token);
        emit(0, token);

        return Jump_backpatch{bytecode_};
    }

    void Chunk::emit_loop(unsigned int loop_begin_bytecode_index, const Source_map_token& token)
    {
        emit(gsl::narrow<std::uint8_t>(Opcode::loop), token);
        emit(0, token);
        emit(0, token);

        const auto jump_distance = gsl::narrow<std::uint16_t>(bytecode_.size() - loop_begin_bytecode_index);
        const auto jump_distance_big_endian = boost::endian::native_to_big(jump_distance);
        reinterpret_cast<std::uint16_t&>(*(bytecode_.end() - 2)) = jump_distance_big_endian;
    }

    std::ostream& operator<<(std::ostream& os, const Chunk& chunk)
    {
        os << "Bytecode:\n";
        for (auto bytecode_iter = chunk.bytecode().cbegin(); bytecode_iter != chunk.bytecode().cend();) {
            const auto bytecode_index = bytecode_iter - chunk.bytecode().cbegin();
            const auto& token = chunk.source_map_tokens().at(bytecode_index);
            const auto opcode = static_cast<Opcode>(*bytecode_iter++);

            // Some opcodes such as closure will print multiple lines.
            std::vector<std::string> lines;
            std::ostringstream line;

            line << std::setw(5) << std::setfill(' ') << bytecode_index << " : " << std::setw(2) << std::setfill('0') << std::setbase(16)
                 << static_cast<int>(opcode) << ' ';

            switch (opcode) {
                default: {
                    throw std::logic_error{"Unexpected opcode."};
                }

                case Opcode::add:
                case Opcode::close_upvalue:
                case Opcode::divide:
                case Opcode::equal:
                case Opcode::false_:
                case Opcode::greater:
                case Opcode::inherit:
                case Opcode::less:
                case Opcode::multiply:
                case Opcode::negate:
                case Opcode::nil:
                case Opcode::not_:
                case Opcode::pop:
                case Opcode::print:
                case Opcode::return_:
                case Opcode::subtract:
                case Opcode::true_: {
                    line << "      " << opcode;
                    break;
                }

                case Opcode::call: {
                    const auto arg_count = *bytecode_iter++;
                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(arg_count) << "    " << opcode << " ("
                         << std::setbase(10) << static_cast<int>(arg_count) << ')';

                    break;
                }

                case Opcode::class_:
                case Opcode::constant:
                case Opcode::define_global:
                case Opcode::get_global:
                case Opcode::get_local:
                case Opcode::get_property:
                case Opcode::get_super:
                case Opcode::get_upvalue:
                case Opcode::method:
                case Opcode::set_global:
                case Opcode::set_local:
                case Opcode::set_property:
                case Opcode::set_upvalue: {
                    const auto lookup_index = *bytecode_iter++;
                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(lookup_index) << "    " << opcode
                         << " [" << std::setbase(10) << static_cast<int>(lookup_index) << ']';

                    break;
                }

                case Opcode::closure: {
                    const auto fn_constant_index = *bytecode_iter++;
                    const auto n_tracked_upvalues = *bytecode_iter++;

                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(fn_constant_index) << ' '
                         << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(n_tracked_upvalues) << ' ' << opcode
                         << " [" << std::setbase(10) << static_cast<int>(fn_constant_index) << "] (" << static_cast<int>(n_tracked_upvalues)
                         << ')';

                    for (auto n_tracked_upvalue = 0; n_tracked_upvalue != n_tracked_upvalues; ++n_tracked_upvalue) {
                        const auto is_direct_capture = *bytecode_iter++;
                        const auto enclosing_index = *bytecode_iter++;

                        lines.push_back(std::move(line).str());
                        line << "           " << std::setw(2) << std::setfill('0') << std::setbase(16)
                             << static_cast<int>(is_direct_capture) << ' ' << std::setw(2) << std::setfill('0') << std::setbase(16)
                             << static_cast<int>(enclosing_index) << " | " << (is_direct_capture ? "^" : "^^") << " ["
                             << static_cast<int>(enclosing_index) << ']';
                    }

                    break;
                }

                case Opcode::jump:
                case Opcode::jump_if_false:
                case Opcode::loop: {
                    const auto jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*bytecode_iter);
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);
                    const auto jump_target = bytecode_index + 3 + jump_distance * (opcode == Opcode::loop ? -1 : 1);

                    const auto jump_distance_high_byte = *bytecode_iter++;
                    const auto jump_distance_low_byte = *bytecode_iter++;

                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(jump_distance_high_byte) << ' '
                         << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(jump_distance_low_byte) << ' '
                         << opcode << ' ' << (opcode == Opcode::loop ? '-' : '+') << std::setbase(10) << jump_distance << " -> "
                         << jump_target;

                    break;
                }
            }

            lines.push_back(std::move(line).str());
            for (const auto& line : lines) {
                os << std::setw(40) << std::setfill(' ') << std::left << line << " ; " << *token.lexeme << " @ " << token.line << '\n';
            }
        }

        os << "Constants:\n";
        if (chunk.constants().empty()) {
            os << "    -\n";
        } else {
            for (auto constant_iter = chunk.constants().cbegin(); constant_iter != chunk.constants().cend(); ++constant_iter) {
                const auto constant_index = constant_iter - chunk.constants().cbegin();
                os << std::setw(5) << std::right << constant_index << " : " << *constant_iter << '\n';
            }
        }

        // Recursively traverse nested functions.
        for (const auto value : chunk.constants()) {
            if (std::holds_alternative<GC_ptr<Function>>(value)) {
                os << '[' << value << " chunk]\n" << std::get<GC_ptr<Function>>(value)->chunk;
            }
        }

        return os;
    }
}
