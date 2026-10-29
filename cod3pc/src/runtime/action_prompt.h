// Action-word interpretation adapted from SirCarls 6ae4839 (MIT notice in licenses/).
#pragma once
#include <string>
#include <string_view>
namespace cod3::input {
struct ActionPrompt { int action = -1; int form = 0; };
inline ActionPrompt ParseActionPrompt(std::string_view text) {
  if (text.size() < 3 || text.front() != '[' || text.back() != ']') return {};
  text.remove_prefix(1); text.remove_suffix(1);
  int form = 0;
  if (text.starts_with("PRESS_")) { form = 1; text.remove_prefix(6); }
  else if (text.starts_with("VERB_")) { form = 2; text.remove_prefix(5); }
  else if (text.starts_with("HOLD_")) { form = 3; text.remove_prefix(5); }
  struct Word { std::string_view name; int action; };
  constexpr Word words[]{ {"RELOAD",0}, {"USE",1}, {"ACTIVATE",1},
    {"ATTACK",4}, {"AIM",5}, {"TOGGLE_AIM",5}, {"WEAPON_SWAP",6},
    {"GRENADE",8}, {"SMOKEGRENADE",9}, {"MELEE",10}, {"BINOCS",11} };
  for (auto word : words) if (text == word.name) return {word.action, form};
  return {};
}
inline std::string FormatActionPrompt(ActionPrompt prompt, std::string_view key,
    std::string_view press, std::string_view hold) {
  if (prompt.action < 0 || key.empty()) return {};
  const auto verb = prompt.form == 3 ? hold : press;
  if (prompt.form && verb.empty()) return {};
  return (prompt.form ? std::string(verb) + " " : "") + "[" + std::string(key) + "]";
}
}
