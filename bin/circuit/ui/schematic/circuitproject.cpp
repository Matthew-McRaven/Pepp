#include "circuitproject.hpp"
#include "schematic/blueprint.hpp"
#include "schematic/blueprintlibrary.hpp"
#include "schematic/circuitschematic.hpp"

namespace {
std::shared_ptr<BuiltinBlueprint> add_2x1(CircuitProject &f, BlueprintLibrary &library, i16 multiplier,
                                          schematic::BlueprintGroupID gid, const std::string &name) {
  using namespace pepp::core;
  auto ret = std::make_shared<BuiltinBlueprint>();
  // Ensure alignment is always non-zero to avoid a /0 in alignment calculations.
  // Dividing by 2 is an arbitrary choice I've made for demo purposes.
  const auto align = std::max<schematic::Coord>(multiplier / 2, 1);
  ret->alignmentConstraint = AlignmentConstraint{.x_modulus = align, .y_modulus = align};
  ret->size =
      schematic::Size{static_cast<schematic::Coord>(3 * multiplier), static_cast<schematic::Coord>(2 * multiplier)};
  ret->pins = {
      Blueprint::Pin{schematic::Rectangle::from_point_size(multiplier * 2 - 1, 0, multiplier * 1, multiplier * 2),
                     PinType::Output},
      Blueprint::Pin{schematic::Rectangle::from_point_size(0, 0, multiplier * 1, multiplier * 1), PinType::Input},
      Blueprint::Pin{schematic::Rectangle::from_point_size(0, multiplier * 1, multiplier * 1, multiplier * 1),
                     PinType::Input},
  };
  auto ret_id = library.add_blueprint(name, ret);
  library.add_blueprint_to_group(gid, ret_id);
  return ret;
}

u32 add_2x1_group(CircuitProject &f, BlueprintLibrary &library, u16 multiplier, const std::string &name,
                  const std::string &path, BuiltinBlueprint::Type type) {
  auto fpath = f.track_file(path);
  auto group = std::make_shared<BlueprintGroup>();
  group->name = name;
  group->image = fpath;
  auto group_id = library.add_group(group);

  auto gate2 = add_2x1(f, library, multiplier, group_id, name + "2x1");
  gate2->type = type;
  gate2->image = fpath;

  return 1;
}
u32 add_and_blueprints(CircuitProject &f, BlueprintLibrary &library, i16 multiplier) {
  return add_2x1_group(f, library, multiplier, "AND", ":/and", BuiltinBlueprint::Type::AND);
}

u32 add_or_blueprints(CircuitProject &f, BlueprintLibrary &library, i16 multiplier) {
  return add_2x1_group(f, library, multiplier, "OR", ":/or", BuiltinBlueprint::Type::OR);
}

u32 add_nand_blueprints(CircuitProject &f, BlueprintLibrary &library, i16 multiplier) {
  return add_2x1_group(f, library, multiplier, "NAND", ":/nand", BuiltinBlueprint::Type::NAND);
}

u32 add_nor_blueprints(CircuitProject &f, BlueprintLibrary &library, i16 multiplier) {
  return add_2x1_group(f, library, multiplier, "NOR", ":/nor", BuiltinBlueprint::Type::NOR);
}

u32 add_xor_blueprints(CircuitProject &f, BlueprintLibrary &library, i16 multiplier) {
  return add_2x1_group(f, library, multiplier, "XOR", ":/xor", BuiltinBlueprint::Type::XOR);
}

} // namespace

CircuitProject::CircuitProject() {
  _schematic = std::make_shared<CircuitSchematic>();
  _library = std::make_shared<BlueprintLibrary>();
}

schematic::ImageFileKey CircuitProject::track_file(const std::string &file_path) {
  if (auto it = _inverse_files.find(file_path); it != _inverse_files.end()) {
    return it->second;
  }

  schematic::ImageFileKey key{_next_file_index++};
  _inverse_files.emplace(file_path, key);
  _files.emplace(key, file_path);
  return key;
}

std::optional<const std::string *> CircuitProject::find_file(schematic::ImageFileKey key) const {
  if (auto it = _files.find(key); it == _files.end()) return std::nullopt;
  else return &it->second;
}

std::optional<schematic::ImageFileKey> CircuitProject::find_file(const std::string &file_path) const {
  if (auto it = _inverse_files.find(file_path); it == _inverse_files.end()) return std::nullopt;
  else return it->second;
}

bool CircuitProject::contains_file(schematic::ImageFileKey key) const { return _files.contains(key); }

bool CircuitProject::contains_file(const std::string &file_path) const { return _inverse_files.contains(file_path); }

u32 CircuitProject::add_builtin_blueprints(i16 multiplier) {
  u32 ret = 0;
  ret += add_and_blueprints(*this, *_library, multiplier);
  ret += add_or_blueprints(*this, *_library, multiplier);
  ret += add_nand_blueprints(*this, *_library, multiplier);
  ret += add_nor_blueprints(*this, *_library, multiplier);
  ret += add_xor_blueprints(*this, *_library, multiplier);
  return ret;
}

void CircuitProject::add_test_data(i16 multiplier) {
  auto and_blueprint = _library->get_blueprint("AND2x1");
  auto and_id = _schematic->place_component(and_blueprint, multiplier * schematic::Point{6, 4}, Direction::Right);
  auto and_component = _schematic->component(*and_id);
  auto and_in0 = and_component->input_pin(0);
  auto and_in1 = and_component->input_pin(1);
  auto or_blueprint = _library->get_blueprint("OR2x1");
  auto or_id = _schematic->place_component(or_blueprint, multiplier * schematic::Point{2, 6}, Direction::Right);
  auto or_component = _schematic->component(*or_id);
  auto or_out = or_component->output_pin(0);
  auto xor_blueprint = _library->get_blueprint("XOR2x1");
  auto xor_id = _schematic->place_component(xor_blueprint, multiplier * schematic::Point{2, 2}, Direction::Right);
  auto xor_component = _schematic->component(*xor_id);
  auto xor_out = xor_component->output_pin(0);
  _schematic->add_connection(xor_out.global_pin_id(), and_in0.global_pin_id());
  _schematic->add_connection(or_out.global_pin_id(), and_in1.global_pin_id());
}
