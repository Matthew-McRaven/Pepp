#pragma once
#include <memory>
#include <optional>

#include "ir/args.hpp"
#include "ir/base.hpp"

namespace asmb::pep10 {
template <typename address_size_t> class dot_export : public ir::linear_line<address_size_t> {
  public:
    dot_export();
    ~dot_export() override = default;
    dot_export(const dot_export &other);
    dot_export &operator=(dot_export other);
    std::shared_ptr<ir::linear_line<address_size_t>> clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t> &code) const override;

    friend void swap(dot_export &first, dot_export &second) {
        using std::swap;
        swap(static_cast<ir::linear_line<address_size_t> &>(first),
             static_cast<ir::linear_line<address_size_t> &>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<ir::symbol_ref_argument<address_size_t>> argument = nullptr;
};

template <typename address_size_t> class dot_scall : public ir::linear_line<address_size_t> {
  public:
    dot_scall();
    ~dot_scall() override = default;
    dot_scall(const dot_scall &other);
    dot_scall &operator=(dot_scall other);
    std::shared_ptr<ir::linear_line<address_size_t>> clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t> &code) const override;

    friend void swap(dot_scall &first, dot_scall &second) {
        using std::swap;
        swap(static_cast<ir::linear_line<address_size_t> &>(first),
             static_cast<ir::linear_line<address_size_t> &>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<ir::symbol_ref_argument<address_size_t>> argument = nullptr;
};

template <typename address_size_t> class dot_uscall : public ir::linear_line<address_size_t> {
  public:
    dot_uscall();
    ~dot_uscall() override = default;
    dot_uscall(const dot_uscall &other);
    dot_uscall &operator=(dot_uscall other);
    std::shared_ptr<ir::linear_line<address_size_t>> clone() const override;

    // linear_line interface
    std::string generate_listing_string() const override;
    std::string generate_source_string() const override;
    void append_object_code(std::vector<uint8_t> &code) const override;

    friend void swap(dot_uscall &first, dot_uscall &second) {
        using std::swap;
        swap(static_cast<ir::linear_line<address_size_t> &>(first),
             static_cast<ir::linear_line<address_size_t> &>(second));
        swap(first.argument, second.argument);
    }

    std::shared_ptr<ir::symbol_ref_argument<address_size_t>> argument = nullptr;
};

}; // End namespace asmb::pep10
#include "directives.tpp"