#include "highlight.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#include "../isa/pep10.hpp"

std::map<asmb::highlight_type, std::string> asmb::pep10::construct_rules() {
    auto isa = isa::pep10::isa_definition::get_definition();
    std::vector<std::string> help;
    std::transform(isa.isa.begin(), isa.isa.end(), std::back_inserter(help),
                   [](auto &it) { return isa::pep10::as_string(it.second->mnemonic); });

    // TODO: Use regexes from tokenizer once they become available.
    std::map<asmb::highlight_type, std::string> rules;
    rules[asmb::highlight_type::kInstructions] = "(" + boost::algorithm::join(help, "|") + ")";
    rules[asmb::highlight_type::kMacro] = "@([A-zÀ-ÖØ-öø-ÿ_][0-9A-zÀ-ÖØ-öø-ÿ_]*)";
    rules[asmb::highlight_type::kDotDirective] =
        "\\.(ALIGN|ASCII|ADRSS|BLOCK|BURN|BYTE|END|EQUATE|WORD|SYCALL|USYCALL|EXPORT)";
    rules[asmb::highlight_type::kSymbolDecl] = "([A-zÀ-ÖØ-öø-ÿ_][0-9A-zÀ-ÖØ-öø-ÿ_]*)(?=:)";
    rules[asmb::highlight_type::kCharQuote] =
        "((\')(?![\'])(([^\'|\\\\]){1}|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))(\'))";
    rules[asmb::highlight_type::kStrQuote] =
        "((\")((([^\"|\\\\])|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))*)(\"))";
    rules[asmb::highlight_type::kComment] = ";.*$";
    rules[asmb::highlight_type::kWarning] = ";WARNING:[\\s].*$";
    rules[asmb::highlight_type::kError] = ";ERROR:[\\s].*$";
    return rules;
}