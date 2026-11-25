// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/read_only_memory.hpp"
#include "ubc/engine/script_fingerprint.hpp"
#include "ubc/engine/script_relocation.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Synthetic zCParser image for offline tests, derived from the Image and
// Fixture helpers of runtime_script_tests.cpp. A named script model is laid
// out into header, symbol table, symbols, names, zSTRING texts and code. No
// game bytes: every index, entry and offset comes from the model layout.
namespace ubc::test {

[[noreturn]] inline void model_error(const char* what, const std::string& name) {
  std::fprintf(stderr, "parser image: %s: %s\n", what, name.c_str());
  std::abort();
}

struct Memory {
  std::vector<std::uint8_t> bytes;
  std::vector<std::pair<std::uint32_t, std::uint32_t>> denied; // [begin, end)
  std::uint32_t header = 0;
  std::size_t reads = 0;
  unsigned header_reads = 0;
  // The n-th complete header read first moves the code end by one byte.
  unsigned change_on_header = 0;

  void put(std::uint32_t address, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      bytes.at(address + i) = static_cast<std::uint8_t>(value >> (8u * i));
  }
  std::uint32_t get(std::uint32_t address) const {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
      value |= static_cast<std::uint32_t>(bytes.at(address + i)) << (8u * i);
    return value;
  }
  void deny(std::uint32_t address, std::uint32_t size) {
    denied.emplace_back(address, address + size);
  }
  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<Memory*>(context);
    ++self.reads;
    if (address > self.bytes.size() || size > self.bytes.size() - address)
      return false;
    for (const auto& range : self.denied)
      if (static_cast<std::uint64_t>(address) + size > range.first && range.second > address)
        return false;
    if (self.header && address == self.header && size == 0x58) {
      ++self.header_reads;
      if (self.change_on_header && self.header_reads == self.change_on_header)
        self.put(self.header + 0x50, self.get(self.header + 0x50) + 1u);
    }
    std::memcpy(output, self.bytes.data() + address, size);
    return true;
  }
  ReadOnlyMemory memory() noexcept { return {this, &read}; }
};

// Mini assembler: symbol operands, calls and labels are resolved by name
// when the model is laid out. Offsets in symbolic are relative to the
// function start and mark every PUSH_INT that carries a symbol index.
class Assembler {
public:
  enum class Fix { none, symbol, entry, label, relative, text, absolute };
  struct Fixup {
    std::uint32_t at;
    Fix kind;
    std::string name;
    std::int64_t value;
  };

  Assembler& op(std::uint8_t opcode) {
    code.push_back(opcode);
    return *this;
  }
  Assembler& raw(std::initializer_list<std::uint8_t> bytes) {
    code.insert(code.end(), bytes);
    return *this;
  }
  Assembler& push_int(std::int32_t value) {
    return word(64, Fix::none, {}, static_cast<std::uint32_t>(value));
  }
  Assembler& push_symbol_int(const std::string& name) {
    symbolic.push_back(size());
    return word(64, Fix::symbol, name);
  }
  Assembler& push_var(const std::string& name) { return word(65, Fix::symbol, name); }
  Assembler& push_inst(const std::string& name) { return word(67, Fix::symbol, name); }
  Assembler& set_instance(const std::string& name) { return word(80, Fix::symbol, name); }
  Assembler& call_external(const std::string& name) { return word(62, Fix::symbol, name); }
  Assembler& push_text(const std::string& text) { return word(65, Fix::text, text); }
  Assembler& push_array(const std::string& name, std::uint8_t element) {
    word(245, Fix::symbol, name);
    return op(element);
  }
  Assembler& call(const std::string& name) { return word(61, Fix::entry, name); }
  // CALL to start(name) + delta; delta 0 would be an ordinary call.
  Assembler& call_offset(const std::string& name, std::int64_t delta) {
    return word(61, Fix::entry, name, 0, delta);
  }
  Assembler& call_address(std::uint32_t target) { return word(61, Fix::absolute, {}, target); }
  Assembler& jump(const std::string& label) { return word(75, Fix::label, label); }
  Assembler& jumpf(const std::string& label) { return word(76, Fix::label, label); }
  // Jump to start + relative, which may lie outside the function.
  Assembler& jump_relative(std::uint8_t opcode, std::int64_t relative) {
    return word(opcode, Fix::relative, {}, 0, relative);
  }
  Assembler& label(const std::string& name) {
    labels[name] = size();
    return *this;
  }
  Assembler& ret() { return op(60); }
  std::uint32_t size() const { return static_cast<std::uint32_t>(code.size()); }

  std::vector<std::uint8_t> code;
  std::vector<Fixup> fixups;
  std::map<std::string, std::uint32_t> labels;
  std::vector<std::uint32_t> symbolic;

private:
  Assembler& word(std::uint8_t opcode, Fix kind, const std::string& name, std::uint32_t value = 0,
                  std::int64_t delta = 0) {
    code.push_back(opcode);
    if (kind != Fix::none)
      fixups.push_back({size(), kind, name, kind == Fix::absolute ? value : delta});
    for (unsigned i = 0; i < 4; ++i)
      code.push_back(static_cast<std::uint8_t>(value >> (8u * i)));
    return *this;
  }
};

struct ScriptSymbol {
  std::string name;
  std::uint32_t type = 2;
  std::uint32_t flags = 0;
  std::uint32_t elements = 1;
  std::uint32_t offset = 0; // FUNC return type or class member offset
  std::string parent;
  std::int32_t value = 0;
  bool function = false;
  Assembler body;
};

struct Script {
  std::vector<ScriptSymbol> symbols;

  ScriptSymbol& data(const std::string& name, std::uint32_t type, std::uint32_t flags,
                     std::uint32_t elements = 1, const std::string& parent = {},
                     std::int32_t value = 0, std::uint32_t offset = 0) {
    ScriptSymbol symbol;
    symbol.name = name;
    symbol.type = type;
    symbol.flags = flags;
    symbol.elements = elements;
    symbol.parent = parent;
    symbol.value = value;
    symbol.offset = offset;
    symbols.push_back(symbol);
    return symbols.back();
  }
  ScriptSymbol& function(const std::string& name, const Assembler& body, std::uint32_t type = 5,
                         std::uint32_t flags = 1, std::uint32_t argc = 0,
                         const std::string& parent = {}, std::uint32_t return_type = 0) {
    auto& symbol = data(name, type, flags, argc, parent, 0, return_type);
    symbol.function = true;
    symbol.body = body;
    return symbol;
  }
  ScriptSymbol& find(const std::string& name) {
    for (auto& symbol : symbols)
      if (symbol.name == name)
        return symbol;
    model_error("unknown symbol", name);
  }
};

struct Layout {
  std::uint32_t symbol_shift = 0; // filler symbols inserted before shift_at
  std::uint32_t shift_at = 1;
  std::uint32_t code_shift = 0; // filler bytes before the first function
  std::uint32_t base = 0x10000; // first byte of table, symbols, names and code
  std::uint32_t gap = 0;        // extra distance between the regions
};

constexpr std::uint32_t metadata(std::uint32_t elements, std::uint32_t type, std::uint32_t flags) {
  return elements | (type << 12u) | (flags << 16u);
}

struct Built {
  static constexpr std::uint32_t parser = 0x1000;
  Memory image;
  std::uint32_t table = 0;
  std::uint32_t symbols = 0;
  std::uint32_t code = 0;
  std::uint32_t count = 0;
  std::uint32_t code_length = 0;
  std::map<std::string, std::uint32_t> index;
  std::map<std::string, std::uint32_t> start;
  std::map<std::string, std::uint32_t> length;
  std::map<std::string, std::vector<std::uint32_t>> symbolic;
  std::map<std::uint32_t, std::string> names;   // named symbols only
  std::map<std::uint32_t, std::string> texts;   // anonymous text constants
  std::map<std::uint32_t, std::string> entries; // code offset -> function

  std::uint32_t symbol_address(std::uint32_t symbol) const { return symbols + symbol * 0x40u; }
  std::uint32_t code_address(const std::string& function) const {
    return code + start.at(function);
  }
  ReadOnlyMemory memory() noexcept { return image.memory(); }
};

inline std::uint32_t align(std::uint32_t value, std::uint32_t unit) {
  return (value + unit - 1u) / unit * unit;
}

inline Built build(const Script& script, const Layout& layout = {}) {
  Built built;
  struct Entry {
    std::string name;
    const ScriptSymbol* model;
    std::string text;
  };
  std::vector<Entry> order;
  for (std::size_t i = 0; i < script.symbols.size(); ++i) {
    if (i == layout.shift_at)
      for (std::uint32_t k = 0; k < layout.symbol_shift; ++k)
        order.push_back({"UBC_FILLER_" + std::to_string(k), nullptr, {}});
    order.push_back({script.symbols[i].name, &script.symbols[i], {}});
  }
  // Anonymous texts follow the named symbols, one per occurrence.
  std::map<std::pair<std::string, std::uint32_t>, std::uint32_t> text_symbols;
  for (const auto& symbol : script.symbols)
    for (const auto& fixup : symbol.body.fixups)
      if (fixup.kind == Assembler::Fix::text) {
        text_symbols[{symbol.name, fixup.at}] = static_cast<std::uint32_t>(order.size());
        order.push_back({"\xff" + std::to_string(order.size() + 10000u), nullptr, fixup.name});
      }
  built.count = static_cast<std::uint32_t>(order.size());
  for (std::uint32_t i = 0; i < built.count; ++i) {
    if (order[i].model) {
      if (built.index.count(order[i].name) == 0)
        built.index[order[i].name] = i;
      built.names[i] = order[i].name;
    } else if (!order[i].text.empty() || order[i].name[0] == '\xff') {
      built.texts[i] = order[i].text;
    } else {
      built.names[i] = order[i].name;
    }
  }

  std::vector<std::uint8_t> code(layout.code_shift, 60);
  for (const auto& symbol : script.symbols) {
    if (!symbol.function)
      continue;
    const auto start = static_cast<std::uint32_t>(code.size());
    built.start[symbol.name] = start;
    built.length[symbol.name] = symbol.body.size();
    built.symbolic[symbol.name] = symbol.body.symbolic;
    built.entries[start] = symbol.name;
    code.insert(code.end(), symbol.body.code.begin(), symbol.body.code.end());
  }
  built.code_length = static_cast<std::uint32_t>(code.size());
  for (const auto& symbol : script.symbols) {
    if (!symbol.function)
      continue;
    const auto start = built.start[symbol.name];
    for (const auto& fixup : symbol.body.fixups) {
      std::uint32_t value = 0;
      switch (fixup.kind) {
      case Assembler::Fix::symbol:
        if (!built.index.count(fixup.name))
          model_error("unknown operand", fixup.name);
        value = built.index[fixup.name];
        break;
      case Assembler::Fix::entry:
        if (!built.start.count(fixup.name))
          model_error("unknown callee", fixup.name);
        value = static_cast<std::uint32_t>(built.start[fixup.name] + fixup.value);
        break;
      case Assembler::Fix::label:
        if (!symbol.body.labels.count(fixup.name))
          model_error("unknown label", fixup.name);
        value = start + symbol.body.labels.at(fixup.name);
        break;
      case Assembler::Fix::relative:
        value = static_cast<std::uint32_t>(start + fixup.value);
        break;
      case Assembler::Fix::text:
        value = text_symbols.at({symbol.name, fixup.at});
        break;
      case Assembler::Fix::absolute:
        value = static_cast<std::uint32_t>(fixup.value);
        break;
      case Assembler::Fix::none:
        break;
      }
      for (unsigned i = 0; i < 4; ++i)
        code[start + fixup.at + i] = static_cast<std::uint8_t>(value >> (8u * i));
    }
  }

  const auto capacity = built.count + 8u;
  built.table = layout.base;
  built.symbols = align(built.table + capacity * 4u + layout.gap, 0x40);
  const auto names = built.symbols + built.count * 0x40u + layout.gap;
  const auto strings = names + built.count * 0x40u;
  std::uint32_t text_bytes = 0;
  for (const auto& entry : order)
    text_bytes += align(static_cast<std::uint32_t>(entry.text.size()) + 1u, 4);
  const auto string_data = strings + built.count * 0x20u;
  built.code = align(string_data + text_bytes + layout.gap, 0x10);
  auto& image = built.image;
  image.bytes.assign(built.code + built.code_length + 0x100u, 0);
  image.header = Built::parser;
  std::copy(code.begin(), code.end(), image.bytes.begin() + built.code);
  image.put(Built::parser + 0x18, built.table);
  image.put(Built::parser + 0x1c, capacity);
  image.put(Built::parser + 0x20, built.count);
  image.put(Built::parser + 0x48, built.code);
  image.put(Built::parser + 0x4c, built.code + 3u); // The current PC is not a filter.
  image.put(Built::parser + 0x50, built.code + built.code_length);
  image.put(Built::parser + 0x54, built.code_length + 64u);

  auto text_cursor = string_data;
  for (std::uint32_t i = 0; i < built.count; ++i) {
    const auto& entry = order[i];
    const auto symbol = built.symbol_address(i);
    const auto name = names + i * 0x40u;
    if (entry.name.size() > 63)
      model_error("name too long", entry.name);
    image.put(built.table + 4u * i, symbol);
    image.put(symbol + 8, name);
    image.put(symbol + 0xc, static_cast<std::uint32_t>(entry.name.size()));
    image.put(symbol + 0x10, static_cast<std::uint32_t>(entry.name.size() + 1));
    std::memcpy(image.bytes.data() + name, entry.name.c_str(), entry.name.size() + 1);
    if (!entry.model) {
      const bool text = entry.name[0] == '\xff';
      image.put(symbol + 0x20, text ? metadata(1, 3, 1) : metadata(1, 2, 0));
      if (text) {
        const auto object = strings + i * 0x20u;
        image.put(symbol + 0x18, object);
        image.put(object + 8, text_cursor);
        image.put(object + 0xc, static_cast<std::uint32_t>(entry.text.size()));
        std::memcpy(image.bytes.data() + text_cursor, entry.text.c_str(), entry.text.size() + 1);
        text_cursor += align(static_cast<std::uint32_t>(entry.text.size()) + 1u, 4);
      }
      continue;
    }
    const auto& model = *entry.model;
    image.put(symbol + 0x18,
              model.function ? built.start[model.name] : static_cast<std::uint32_t>(model.value));
    image.put(symbol + 0x1c, model.offset);
    image.put(symbol + 0x20, metadata(model.elements, model.type, model.flags));
    if (!model.parent.empty()) {
      if (!built.index.count(model.parent))
        model_error("unknown parent", model.parent);
      image.put(symbol + 0x38, built.symbol_address(built.index[model.parent]));
    }
  }
  return built;
}

struct Recipe {
  std::uint32_t recipe;
  std::vector<std::string> functions;
  std::vector<std::string> data;
};

// Test-side generator: derives a reference profile from a reference build
// in the same way the offline generator will, using only model facts.
class Reference {
public:
  Reference(Built& built, const Script& script, const std::vector<Recipe>& recipes,
            const std::vector<std::string>& ignore_text = {}) {
    std::vector<std::string> sorted;
    for (const auto& item : built.index)
      sorted.push_back(item.first);
    std::sort(sorted.begin(), sorted.end());
    texts_ = sorted;
    for (const auto& text : texts_)
      names_.push_back({text.c_str(), built.index.at(text)});
    const auto id = [this](const std::string& text) {
      const auto found = std::lower_bound(texts_.begin(), texts_.end(), text);
      if (found == texts_.end() || *found != text)
        model_error("name outside the reference", text);
      return static_cast<std::uint32_t>(found - texts_.begin());
    };
    const auto parent = [&id](const std::string& text) {
      return text.empty() ? script_relocation::no_name : id(text);
    };
    std::map<std::string, std::uint32_t> function_ids;
    std::map<std::string, std::uint32_t> data_ids;
    for (const auto& symbol : script.symbols) {
      if (symbol.function) {
        function_ids[symbol.name] = static_cast<std::uint32_t>(symbolic_.size());
        symbolic_.push_back(built.symbolic.at(symbol.name));
      } else {
        data_ids[symbol.name] = static_cast<std::uint32_t>(data_.size());
        script_relocation::DataRecord record{id(symbol.name), symbol.type, symbol.flags,
                                             symbol.elements, parent(symbol.parent)};
        record.check_offset = (symbol.flags & 4u) != 0;
        record.offset = static_cast<std::int32_t>(symbol.offset);
        record.check_value = symbol.type == 2 && symbol.flags == 1 && symbol.elements == 1;
        record.value = symbol.value;
        data_.push_back(record);
      }
    }
    script_fingerprint::Scratch scratch;
    for (const auto& symbol : script.symbols) {
      if (!symbol.function)
        continue;
      const auto& offsets = symbolic_[function_ids.at(symbol.name)];
      const bool ignore =
          std::find(ignore_text.begin(), ignore_text.end(), symbol.name) != ignore_text.end();
      script_fingerprint::Header header{};
      header.name = symbol.name;
      header.type = symbol.type;
      header.flags = symbol.flags;
      header.argc = symbol.elements;
      header.return_type = symbol.type == 5 ? symbol.offset : 0u;
      header.parent = symbol.parent;
      const script_fingerprint::Spec spec{built.length.at(symbol.name), offsets.data(),
                                          offsets.size(), ignore};
      const script_fingerprint::Resolver resolver{&built, &resolve_symbol, &resolve_entry};
      const auto result = script_fingerprint::fingerprint(
          built.memory(), built.code, built.start.at(symbol.name), header, spec, resolver, scratch);
      if (!result.valid())
        model_error(script_fingerprint::issue_name(result.issue), symbol.name);
      script_relocation::FunctionRecord record{id(symbol.name), spec.byte_length, result.canonical,
                                               built.start.at(symbol.name), result.raw};
      record.type = symbol.type;
      record.flags = symbol.flags;
      record.argc = symbol.elements;
      record.return_type = header.return_type;
      record.parent = parent(symbol.parent);
      record.symbolic_ints = offsets.data();
      record.symbolic_int_count = offsets.size();
      record.ignore_text = ignore;
      functions_.push_back(record);
    }
    for (const auto& recipe : recipes) {
      group_functions_.emplace_back();
      group_data_.emplace_back();
      for (const auto& entry : recipe.functions)
        group_functions_.back().push_back(function_ids.at(entry));
      for (const auto& entry : recipe.data)
        group_data_.back().push_back(data_ids.at(entry));
    }
    for (std::size_t i = 0; i < recipes.size(); ++i)
      groups_.push_back({recipes[i].recipe, group_functions_[i].data(), group_functions_[i].size(),
                         group_data_[i].data(), group_data_[i].size()});
  }
  Reference(const Reference&) = delete;
  Reference& operator=(const Reference&) = delete;

  script_relocation::ReferenceProfile profile() const {
    return {Built::parser, names_.data(), names_.size(),  functions_.data(), functions_.size(),
            data_.data(),  data_.size(),  groups_.data(), groups_.size()};
  }
  std::uint32_t function_id(const std::string& text) const {
    for (std::size_t i = 0; i < functions_.size(); ++i)
      if (texts_[functions_[i].name] == text)
        return static_cast<std::uint32_t>(i);
    model_error("unknown function record", text);
  }
  std::vector<script_relocation::FunctionRecord>& functions() { return functions_; }
  std::vector<script_relocation::Name>& names() { return names_; }

private:
  static script_fingerprint::Issue resolve_symbol(void* context, std::uint32_t index,
                                                  script_fingerprint::Operand& operand) noexcept {
    const auto& built = *static_cast<const Built*>(context);
    const auto text = built.texts.find(index);
    if (text != built.texts.end()) {
      Sha256 hash;
      hash.update(text->second.data(), text->second.size());
      operand.kind = script_fingerprint::OperandKind::text;
      operand.text_length = static_cast<std::uint32_t>(text->second.size());
      operand.text_sha256 = hash.finish();
      return script_fingerprint::Issue::none;
    }
    const auto name = built.names.find(index);
    if (name == built.names.end())
      return script_fingerprint::Issue::unknown_symbol;
    operand.name = name->second;
    return script_fingerprint::Issue::none;
  }
  static script_fingerprint::Issue resolve_entry(void* context, std::uint32_t offset,
                                                 script_fingerprint::Operand& operand) noexcept {
    const auto& built = *static_cast<const Built*>(context);
    const auto entry = built.entries.find(offset);
    if (entry == built.entries.end())
      return script_fingerprint::Issue::unknown_callee;
    operand.name = entry->second;
    return script_fingerprint::Issue::none;
  }

  std::vector<std::string> texts_;
  std::vector<script_relocation::Name> names_;
  std::vector<std::vector<std::uint32_t>> symbolic_;
  std::vector<script_relocation::FunctionRecord> functions_;
  std::vector<script_relocation::DataRecord> data_;
  std::vector<std::vector<std::uint32_t>> group_functions_;
  std::vector<std::vector<std::uint32_t>> group_data_;
  std::vector<script_relocation::RecipeGroup> groups_;
};

} // namespace ubc::test
