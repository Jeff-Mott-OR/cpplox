#include "chunk.hpp"

#include <iomanip>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/endian/conversion.hpp>
#include <gsl/gsl>

#include "object.hpp"

namespace motts { namespace lox {
    std::ostream& operator<<(std::ostream& os, const Opcode& opcode) {
        std::string name {[&] {
            switch (opcode) {
                default:
                    throw std::logic_error{"Unexpected opcode."};

                #define X(name) \
                    case Opcode::name: \
                        return #name;
                MOTTS_LOX_OPCODE_NAMES
                #undef X
            }
        }()};

        // Names should print as uppercase without trailing underscores.
        boost::trim_right_if(name, boost::is_any_of("_"));
        boost::to_upper(name);

        os << name;

        return os;
    }

    Chunk::Jump_backpatch::Jump_backpatch(Bytecode_vector& bytecode)
        : bytecode_ {bytecode},
          jump_begin_index_ {bytecode.size()}
    {}

    void Chunk::Jump_backpatch::to_next_opcode() {
        const auto jump_distance = gsl::narrow<std::uint16_t>(bytecode_.size() - jump_begin_index_);
        const auto jump_distance_big_endian = boost::endian::native_to_big(jump_distance);
        reinterpret_cast<std::uint16_t&>(bytecode_.at(jump_begin_index_ - 2)) = jump_distance_big_endian;
    }

    Chunk::Constants_vector::size_type Chunk::insert_constant(Dynamic_type_value&& constant_value) {
        const auto maybe_constant_iter = std::find(constants_.cbegin(), constants_.cend(), constant_value);
        if (maybe_constant_iter != constants_.cend()) {
            const auto constant_index = maybe_constant_iter - constants_.cbegin();
            return gsl::narrow<Constants_vector::size_type>(constant_index);
        }

        const auto constant_index = constants_.size();
        constants_.push_back(std::move(constant_value));

        return constant_index;
    }

    template<Opcode opcode>
        void Chunk::emit(const Token& source_map_token) {
            bytecode_.push_back(gsl::narrow<std::uint8_t>(opcode));
            source_map_tokens_.push_back(source_map_token);
        }

    // Explicit instantiation of the opcodes allowed with this templated member function.
    // `emit<Opcode::constant>`, for example, should *not* be accepted through this template.
    template void Chunk::emit<Opcode::add>(const Token&);
    template void Chunk::emit<Opcode::close_upvalue>(const Token&);
    template void Chunk::emit<Opcode::divide>(const Token&);
    template void Chunk::emit<Opcode::false_>(const Token&);
    template void Chunk::emit<Opcode::equal>(const Token&);
    template void Chunk::emit<Opcode::greater>(const Token&);
    template void Chunk::emit<Opcode::less>(const Token&);
    template void Chunk::emit<Opcode::multiply>(const Token&);
    template void Chunk::emit<Opcode::negate>(const Token&);
    template void Chunk::emit<Opcode::nil>(const Token&);
    template void Chunk::emit<Opcode::not_>(const Token&);
    template void Chunk::emit<Opcode::pop>(const Token&);
    template void Chunk::emit<Opcode::print>(const Token&);
    template void Chunk::emit<Opcode::return_>(const Token&);
    template void Chunk::emit<Opcode::subtract>(const Token&);
    template void Chunk::emit<Opcode::true_>(const Token&);

    template<Opcode opcode>
        void Chunk::emit(const Token& variable_name, const Token& source_map_token) {
            const auto constant_index = insert_constant(Dynamic_type_value{std::string{variable_name.lexeme}});

            bytecode_.push_back(gsl::narrow<std::uint8_t>(opcode));
            bytecode_.push_back(gsl::narrow<std::uint8_t>(constant_index));

            source_map_tokens_.push_back(source_map_token);
            source_map_tokens_.push_back(source_map_token);
        }

    template void Chunk::emit<Opcode::define_global>(const Token&, const Token&);
    template void Chunk::emit<Opcode::get_global>(const Token&, const Token&);
    template void Chunk::emit<Opcode::set_global>(const Token&, const Token&);

    template<Opcode opcode>
        void Chunk::emit(int local_stack_index, const Token& source_map_token) {
            bytecode_.push_back(gsl::narrow<std::uint8_t>(opcode));
            bytecode_.push_back(gsl::narrow<std::uint8_t>(local_stack_index));

            source_map_tokens_.push_back(source_map_token);
            source_map_tokens_.push_back(source_map_token);
        }

    template void Chunk::emit<Opcode::get_local>(int, const Token&);
    template void Chunk::emit<Opcode::set_local>(int, const Token&);
    template void Chunk::emit<Opcode::get_upvalue>(int, const Token&);
    template void Chunk::emit<Opcode::set_upvalue>(int, const Token&);

    void Chunk::emit_call(int arg_count, const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::call));
        bytecode_.push_back(gsl::narrow<std::uint8_t>(arg_count));

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_closure(
        const GC_ptr<Function>& function,
        const std::vector<Tracked_upvalue>& tracked_upvalues,
        const Token& source_map_token
    ) {
        const auto fn_constant_index = insert_constant(Dynamic_type_value{function});

        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::closure));
        bytecode_.push_back(gsl::narrow<std::uint8_t>(fn_constant_index));
        bytecode_.push_back(gsl::narrow<std::uint8_t>(tracked_upvalues.size()));

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);

        for (const auto& tracked_upvalue : tracked_upvalues) {
            bytecode_.push_back(gsl::narrow<std::uint8_t>(tracked_upvalue.is_direct_capture ? 1 : 0));
            bytecode_.push_back(gsl::narrow<std::uint8_t>(tracked_upvalue.enclosing_index));

            source_map_tokens_.push_back(source_map_token);
            source_map_tokens_.push_back(source_map_token);
        }
    }

    void Chunk::emit_constant(Dynamic_type_value&& constant_value, const Token& source_map_token) {
        const auto constant_index = insert_constant(std::move(constant_value));

        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::constant));
        bytecode_.push_back(gsl::narrow<std::uint8_t>(constant_index));

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    Chunk::Jump_backpatch Chunk::emit_jump(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::jump));
        bytecode_.push_back(0);
        bytecode_.push_back(0);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);

        return Jump_backpatch{bytecode_};
    }

    Chunk::Jump_backpatch Chunk::emit_jump_if_false(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::jump_if_false));
        bytecode_.push_back(0);
        bytecode_.push_back(0);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);

        return Jump_backpatch{bytecode_};
    }

    void Chunk::emit_loop(int loop_begin_bytecode_index, const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::loop));
        bytecode_.push_back(0);
        bytecode_.push_back(0);

        const auto jump_distance = gsl::narrow<std::uint16_t>(bytecode_.size() - loop_begin_bytecode_index);
        const auto jump_distance_big_endian = boost::endian::native_to_big(jump_distance);
        reinterpret_cast<std::uint16_t&>(*(bytecode_.end() - 2)) = jump_distance_big_endian;

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    std::ostream& operator<<(std::ostream& os, const Chunk& chunk) {
        os << "Constants:\n";
        for (auto constant_iter = chunk.constants().cbegin(); constant_iter != chunk.constants().cend(); ++constant_iter) {
            const auto constant_index = constant_iter - chunk.constants().cbegin();
            os << std::setw(5) << std::right << constant_index << " : " << *constant_iter << '\n';
        }

        os << "Bytecode:\n";
        for (auto bytecode_iter = chunk.bytecode().cbegin(); bytecode_iter != chunk.bytecode().cend(); ) {
            std::vector<std::ostringstream> lines;
            lines.push_back({});

            const auto bytecode_index = bytecode_iter - chunk.bytecode().cbegin();
            const auto opcode = static_cast<Opcode>(*bytecode_iter);

            lines.back() << std::setw(5) << std::setfill(' ') << bytecode_index
                << " : "
                << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(opcode)
                << " ";

            switch (opcode) {
                default:
                    throw std::logic_error{"Unexpected opcode."};

                case Opcode::add:
                case Opcode::close_upvalue:
                case Opcode::divide:
                case Opcode::equal:
                case Opcode::false_:
                case Opcode::greater:
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
                    lines.back() << std::setw(6) << std::setfill(' ') << ' ' << opcode;
                    bytecode_iter += 1;
                    break;
                }

                case Opcode::call:
                case Opcode::constant:
                case Opcode::define_global:
                case Opcode::get_global:
                case Opcode::get_local:
                case Opcode::get_upvalue:
                case Opcode::set_global:
                case Opcode::set_local:
                case Opcode::set_upvalue: {
                    lines.back() << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 1))
                        << "    " << opcode << " [" << std::setbase(10) << static_cast<int>(*(bytecode_iter + 1)) << ']';
                    bytecode_iter += 2;
                    break;
                }

                case Opcode::closure: {
                    const auto& fn_constant_index = *(bytecode_iter + 1);
                    const auto& n_tracked_upvalues = *(bytecode_iter + 2);

                    lines.back() << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(fn_constant_index) << ' '
                        << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(n_tracked_upvalues) << ' '
                        << opcode << " [" << std::setbase(10) << static_cast<int>(fn_constant_index) << "] (" << static_cast<int>(n_tracked_upvalues) << ')';

                    for (auto n_tracked_upvalue = 0; n_tracked_upvalue != n_tracked_upvalues; ++n_tracked_upvalue) {
                        const auto& is_direct_capture = *(bytecode_iter + 3 + 2 * n_tracked_upvalue);
                        const auto& enclosing_index = *(bytecode_iter + 3 + 2 * n_tracked_upvalue + 1);

                        lines.push_back({});
                        lines.back() << std::setw(11) << std::setfill(' ') << ' '
                            << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(is_direct_capture) << ' '
                            << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(enclosing_index)
                            << " | " << (is_direct_capture ? "^" : "^^") << " [" << static_cast<int>(enclosing_index) << ']';
                    }

                    bytecode_iter += 3 + 2 * n_tracked_upvalues;

                    break;
                }

                case Opcode::jump:
                case Opcode::jump_if_false:
                case Opcode::loop: {
                    const auto jump_distance = boost::endian::big_to_native(reinterpret_cast<const std::uint16_t&>(*(bytecode_iter + 1)));
                    const auto jump_target = bytecode_index + 3 + (opcode == Opcode::loop ? -1 : 1) * jump_distance;

                    lines.back() << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 1)) << ' '
                        << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 2)) << ' '
                        << opcode << ' '
                        << (opcode == Opcode::loop ? '-' : '+') << std::setbase(10) << jump_distance
                        << " -> " << jump_target;

                    bytecode_iter += 3;
                    break;
                }
            }

            const auto& source_map_token = chunk.source_map_tokens().at(bytecode_index);
            for (const auto& line : lines) {
                os << std::setw(40) << std::setfill(' ') << std::left << line.str() << " ; "
                    << source_map_token.lexeme << " @ " << source_map_token.line << '\n';
            }
        }

        for (const auto& constant_dynamic_value : chunk.constants()) {
            if (std::holds_alternative<GC_ptr<Function>>(constant_dynamic_value.variant)) {
                os << "## " << constant_dynamic_value << " chunk\n"
                    << std::get<GC_ptr<Function>>(constant_dynamic_value.variant)->chunk;
            }
        }

        return os;
    }

    bool operator==(const Chunk::Tracked_upvalue& lhs, const Chunk::Tracked_upvalue& rhs) {
        return lhs.is_direct_capture == rhs.is_direct_capture && lhs.enclosing_index == rhs.enclosing_index;
    }
}}
