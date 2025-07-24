#include "trace_tags.hpp"
#include "./stack_ops.hpp"
#include "CXXGraph/CXXGraph.hpp"
#include "expr_rtti.hpp"
#include "toolchain/pas/operations/generic/trace_tags.hpp"
#include "zpp_bits.h"

static const auto traceStr = ".debug_trace";

std::optional<pepp::debug::types::Primitives> primitive_from_arg(const QStringView &view) {
  using enum pepp::debug::types::Primitives;
  static const auto case_ins = Qt::CaseSensitivity::CaseInsensitive;
  if (view.compare("2h", case_ins) == 0) return u16;
  else if (view.compare("2u", case_ins) == 0) return u16;
  else if (view.compare("2d", case_ins) == 0) return i16;
  else if (view.compare("2s", case_ins) == 0) return i16;
  else if (view.compare("1h", case_ins) == 0) return u8;
  else if (view.compare("1u", case_ins) == 0) return u8;
  else if (view.compare("1d", case_ins) == 0) return i8;
  else if (view.compare("1s", case_ins) == 0) return i8;
  else if (view.compare("1c", case_ins) == 0) return i8;
  return std::nullopt;
}
using cmd_list = decltype(pas::ops::generic::extractTraceTags(std::declval<pas::ast::Node &>()));
using IndirectHandle = pepp::debug::types::TypeInfo::IndirectHandle;
using CmdIterator = cmd_list::iterator;

// Add named nodes into the graph for the struct and all of its members.
// Add edge from member name to struct name.
// Member name should not have any "in" edges unless it is a struct itself.
void updateStructGraph(CmdIterator &it, pepp::debug::types::TypeInfo &info, CXXGraph::Graph<IndirectHandle> &depends_on,
                       CXXGraph::id_t &nextID) {

  auto indirect = info.register_indirect(*it->symbolDecl);
  std::shared_ptr<const CXXGraph::Node<IndirectHandle>> struct_node;
  // Create node for struct or reusue on if it exists
  if (auto node = depends_on.getNode((it->symbolDecl)->toStdString()); node.has_value()) struct_node = *node;
  else {
    struct_node = std::make_shared<CXXGraph::Node<IndirectHandle>>((it->symbolDecl)->toStdString(), indirect.second);
    depends_on.addNode(struct_node);
  }

  // For each named member, create or reuse a node for it.
  for (const auto &member : std::as_const(it->command.args)) {
    auto indirect_from_member = info.register_indirect(member);
    std::shared_ptr<const CXXGraph::Node<IndirectHandle>> member_node;
    if (auto node = depends_on.getNode(member.toStdString()); node.has_value()) member_node = *node;
    else {
      member_node = std::make_shared<CXXGraph::Node<IndirectHandle>>(member.toStdString(), indirect_from_member.second);
      depends_on.addNode(member_node);
    }

    // Add a dependency from the member name to the struct name.
    [[maybe_unused]] CXXGraph::id_t edge_id = 0;
    if (!depends_on.findEdge(member_node, struct_node, edge_id)) {
      auto edge = std::make_shared<CXXGraph::DirectedEdge<IndirectHandle>>(nextID++, member_node, struct_node);
      depends_on.addEdge(edge);
    }
  }
}

// Remove nodes from depends_on until no more nodes can be removed. Nodes with no in edges are candidates for removal.
// Used to define structs after their members have been defined.
void createTypesAndContractGraph(pepp::debug::types::TypeInfo &info, CXXGraph::Graph<IndirectHandle> &depends_on,
                                 QMap<std::string, QStringList> &deferredStructs) {
  auto nodes = depends_on.getNodeSet();
  auto last_size = nodes.size();
  // Now handle deferred structs
  do {
    last_size = nodes.size();
    // Remove all nodes which only have out edges
    for (const auto &node : nodes) {
      auto inout = depends_on.inOutEdges(node);
      // If the node has no in-edges, it is a root node.
      bool is_root = true;
      for (const auto &edge : inout) {
        if (edge->getNodePair().second == node) {
          is_root = false;
          break;
        }
      }
      // Not a root node? Can't be removed or handled yet.
      if (!is_root) continue;
      // If the node has no out-edges, and all of its dependencies are already registered.
      auto ihnd = node->getData();
      // Leaf nodes might be non-struct types; don't try to register them as structs.
      if (deferredStructs.contains(node->getUserId())) {
        auto members = deferredStructs[node->getUserId()];
        quint16 offset = 0;
        std::vector<std::tuple<QString, pepp::debug::types::BoxedType, quint16>> members_list;
        for (const auto &member : std::as_const(members)) {
          auto member_handle = info.register_indirect(member);
          auto boxed_type = info.type_from(member_handle.second);
          members_list.emplace_back(member, boxed_type, offset);
          // Adjust offset to accomodate size of member.
          offset += pepp::debug::types::bitness(unbox(boxed_type)) / 8;
        }
        pepp::debug::types::Type _struct = pepp::debug::types::Struct{2, members_list};
        auto dhnd = info.register_direct(_struct);
        info.set_indirect_type(ihnd, dhnd);
      }
      depends_on.removeNode(node->getUserId()); // First removal puts it in isolated node list.
      depends_on.removeNode(node->getUserId()); // Second removal destroys the node.
    }
    nodes = depends_on.getNodeSet();
  } while (last_size != nodes.size() && nodes.size() != 0);
  if (nodes.size() != 0) {
    qDebug() << "Failed to resolve all nodes in dependency graph";
    std::cout << depends_on;
  }
}

using CommandMap = QMap<quint32, pepp::debug::CommandFrame>;
void parseNonGlobal(CmdIterator &it, pepp::debug::types::TypeInfo &info, CommandMap &commands) {

  const auto &cmd = it->command.command;
  const auto &args = it->command.args;
  using namespace pepp::debug;
  auto packet = pepp::debug::CommandPacket{};
  bool is_push = it->isPush;

  if (cmd == "call" || cmd == "ret") {
    Opcodes opcode = cmd == "call" ? Opcodes::CALL : Opcodes::RET;
    static const QString name = "retAddr";
    auto [_, ihnd] = info.register_indirect(name);
    auto type = info.box(pepp::debug::types::Primitives::u16);
    packet.ops.emplace_back(StackOp{MemoryOp{opcode, name, type}});
  } else if (cmd == "locals" || cmd == "param") {
    // Add stack frame init or deinit ops, since params create/delete a stack frame.
    if (cmd == "param") {
      if (is_push) packet.ops.emplace_back(StackOp{FrameManagement{Opcodes::ADD_FRAME}});
      else packet.ops.emplace_back(StackOp{FrameActive{false}});
    }

    const auto opcode = is_push ? Opcodes::PUSH : Opcodes::POP;
    for (const auto &arg : std::as_const(args)) {
      auto [success, ihnd] = info.register_indirect(arg);
      if (success) { // Type should already be registered
        qDebug() << "Did not find type for: " << arg;
        continue;
      }
      auto boxed_type = info.type_from(ihnd);
      auto unboxed = unbox(boxed_type);
      auto dhnd = info.get_direct(unboxed);
      packet.ops.emplace_back(StackOp{MemoryOp{opcode, arg, boxed_type}});
    }

    // Add stack frame init or deinit ops, since params create/delete a stack frame.
    if (cmd == "param") {
      if (is_push) packet.ops.emplace_back(StackOp{FrameActive{true}});
      else packet.ops.emplace_back(StackOp{FrameManagement{Opcodes::REMOVE_FRAME}});
    }
  } else return;

  if (const auto maybe_address = it->address; maybe_address) {
    auto address = *maybe_address;
    if (!commands.contains(address)) commands[address] = {};
    commands[address].packets.push_back(packet);
  }
}

[[nodiscard]] std::errc serialize(auto &archive, auto &commands, pepp::debug::types::SerializationHelper &h) {
  using archive_type = std::remove_cvref_t<decltype(archive)>;
  if constexpr (archive_type::kind() == zpp::bits::kind::out) {
    if (auto errc = archive((quint16)commands.size()); errc.code != std::errc()) return errc;
    for (const auto &[addr, frame] : commands.asKeyValueRange()) {
      if (auto errc = archive(addr); errc.code != std::errc()) return errc;
      if (auto errc = frame.serialize(archive, frame, h); errc.code != std::errc()) return errc;
    }
  } else if constexpr (archive_type::kind() == zpp::bits::kind::in &&
                       !std::is_const_v<std::remove_reference_t<decltype(commands)>>) {
    quint16 size = 0;
    if (auto errc = archive(size); errc.code != std::errc()) return errc;
    for (int i = 0; i < size; ++i) {
      quint32 addr = 0;
      if (auto errc = archive(addr); errc.code != std::errc()) return errc;
      pepp::debug::CommandFrame frame;
      if (auto errc = frame.serialize(archive, frame, h); errc.code != std::errc()) return errc;
      commands[addr] = std::move(frame);
    }
  }
  return std::errc{};
}

void pas::obj::common::writeDebugCommands(ELFIO::elfio &elf, std::list<ast::Node *> roots) {
  static const auto is_type_decl = [](const ops::generic::Command &cmd) {
    auto str = cmd.command.command;
    return str == "type" || str == "struct";
  };
  static const auto is_global_decl = [](const ops::generic::Command &cmd) {
    auto str = cmd.command.command;
    return str == "global";
  };
  auto trace = detail::getOrAddTraceSection(elf);
  auto [data, in, out] = zpp::bits::data_in_out();

  cmd_list type_decls, global_decls, rest_decls;
  pepp::debug::types::TypeInfo info;

  // Partition trace commands for all roots between our three kinds of command lists.
  for (const auto &root : roots) {
    auto tt = ops::generic::extractTraceTags(*root);
    for (auto it = tt.begin(); it != tt.end();) {
      if (is_type_decl(*it)) it = (type_decls.push_back(std::move(*it)), tt.erase(it));
      else if (is_global_decl(*it)) it = (global_decls.push_back(std::move(*it)), tt.erase(it));
      else it = (rest_decls.push_back(std::move(*it)), tt.erase(it));
    }
  }

  CXXGraph::Graph<IndirectHandle> depends_on;
  CXXGraph::id_t nextID = 0;
  QMap<std::string, QStringList> deferredStructs;

  // for (const auto &cmd : as_const(type_decls)) qDebug().noquote() << cmd;
  // for (const auto &cmd : as_const(global_decls)) qDebug().noquote() << cmd;
  // for (const auto &cmd : as_const(rest_decls)) qDebug().noquote() << cmd;

  // Perform type declarations immediately, and produce a directed graph of struct definitions.
  for (auto it = type_decls.begin(); it != type_decls.end(); it++) {
    if (!it->symbolDecl.has_value()) qDebug() << "Type decl had empty symbol" << *it;
    if (it->command.command == "type" && it->command.args.size() == 1) {
      auto prim = primitive_from_arg(it->command.args[0]);
      if (!prim.has_value()) {
        qDebug() << "Unknown primitive type in" << it->command.command << it->command.args[0];
        continue;
      }
      auto direct = info.register_direct(*prim);
      auto indirect = info.register_indirect(it->symbolDecl.value());
      info.set_indirect_type(indirect.second, direct);
    } else if (it->command.command == "struct") {
      updateStructGraph(it, info, depends_on, nextID);
      deferredStructs[it->symbolDecl.value().toStdString()].append(it->command.args);
    } else {
      qDebug() << "Unhandled type declaration command:" << it->command;
    }
  }
  // Resolve directed graph into Struct{} types.
  createTypesAndContractGraph(info, depends_on, deferredStructs);

  // Parse local/params into command packets.
  CommandMap commands;
  for (auto it = rest_decls.begin(); it != rest_decls.end(); it++) parseNonGlobal(it, info, commands);

  qDebug() << info;
  pepp::debug::types::SerializationHelper h;
  // TODO: extract all strings from commands into h. Info will serialize them for us. Then we can use those indices in
  // stack_ops serialization.
  (void)info.serialize(out, info, &h);
  (void)serialize(out, commands, h);
  for (auto addr = commands.keyBegin(); addr != commands.keyEnd(); addr++)
    qDebug().noquote() << *addr << commands[*addr];
  trace->append_data((const char *)data.data(), data.size());
}

pas::obj::common::DebugInfo pas::obj::common::readDebugCommands(ELFIO::elfio &elf) {
  DebugInfo ret;
  auto trace = detail::getTraceSection(elf);
  if (trace == nullptr) {
    qDebug() << "No trace section found in ELF file";
    return ret;
  } else qDebug() << "Found valid trace sections";

  auto span = std::span<const char>(trace->get_data(), trace->get_stream_size());
  auto in = zpp::bits::input(span);
  pepp::debug::types::SerializationHelper h;
  ret.typeInfo = std::make_shared<pepp::debug::types::TypeInfo>();

  qDebug() << *ret.typeInfo;
  if (auto errc = ret.typeInfo->serialize(in, *ret.typeInfo, &h); errc != std::errc())
    throw std::logic_error("Failed to deserialize type info from trace section");
  qDebug() << *ret.typeInfo;

  if (auto errc = serialize(in, ret.commands, h); errc != std::errc())
    throw std::logic_error("Failed to deserialize command packets");
  for (auto addr = ret.commands.keyBegin(); addr != ret.commands.keyEnd(); addr++)
    qDebug().noquote() << *addr << ret.commands[*addr];
  return ret;
}

ELFIO::section *pas::obj::common::detail::getTraceSection(ELFIO::elfio &elf) {
  ELFIO::section *trace = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_name() == traceStr && sec->get_type() == ELFIO::SHT_PROGBITS) {
      trace = sec.get();
      break;
    }
  }
  return trace;
}

ELFIO::section *pas::obj::common::detail::getOrAddTraceSection(ELFIO::elfio &elf) {
  auto trace = getTraceSection(elf);
  if (trace == nullptr) {
    trace = elf.sections.add(traceStr);
    trace->set_type(ELFIO::SHT_PROGBITS);
  }
  return trace;
}
