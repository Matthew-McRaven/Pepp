/*
 * .EXPORT
 */
template <typename address_size_t> asmb::pep10::dot_export<address_size_t>::dot_export() {}

template <typename address_size_t>
asmb::pep10::dot_export<address_size_t>::dot_export(const asmb::pep10::dot_export<address_size_t> &other) {}

template <typename address_size_t>
asmb::pep10::dot_export<address_size_t> &
asmb::pep10::dot_export<address_size_t>::operator=(asmb::pep10::dot_export<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<ir::linear_line<address_size_t>> asmb::pep10::dot_export<address_size_t>::clone() const {
    return std::make_shared<dot_export<address_size_t>>(*this);
}

template <typename address_size_t>
std::string asmb::pep10::dot_export<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "", "", generate_source_string());

    return temp;
}

template <typename address_size_t> std::string asmb::pep10::dot_export<address_size_t>::generate_source_string() const {
    auto dot_string = ".EXPORT";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void asmb::pep10::dot_export<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

/*
 * .scall
 */
template <typename address_size_t> asmb::pep10::dot_scall<address_size_t>::dot_scall() {}

template <typename address_size_t>
asmb::pep10::dot_scall<address_size_t>::dot_scall(const asmb::pep10::dot_scall<address_size_t> &other) {}

template <typename address_size_t>
asmb::pep10::dot_scall<address_size_t> &
asmb::pep10::dot_scall<address_size_t>::operator=(asmb::pep10::dot_scall<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<ir::linear_line<address_size_t>> asmb::pep10::dot_scall<address_size_t>::clone() const {
    return std::make_shared<dot_scall<address_size_t>>(*this);
}

template <typename address_size_t> std::string asmb::pep10::dot_scall<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "", "", generate_source_string());

    return temp;
}

template <typename address_size_t> std::string asmb::pep10::dot_scall<address_size_t>::generate_source_string() const {
    auto dot_string = ".SCALL";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void asmb::pep10::dot_scall<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}

/*
 * .uscall
 */
template <typename address_size_t> asmb::pep10::dot_uscall<address_size_t>::dot_uscall() {}

template <typename address_size_t>
asmb::pep10::dot_uscall<address_size_t>::dot_uscall(const asmb::pep10::dot_uscall<address_size_t> &other) {}

template <typename address_size_t>
asmb::pep10::dot_uscall<address_size_t> &
asmb::pep10::dot_uscall<address_size_t>::operator=(asmb::pep10::dot_uscall<address_size_t> other) {
    swap(*this, other);
    return *this;
}

template <typename address_size_t>
std::shared_ptr<ir::linear_line<address_size_t>> asmb::pep10::dot_uscall<address_size_t>::clone() const {
    return std::make_shared<dot_uscall<address_size_t>>(*this);
}

template <typename address_size_t>
std::string asmb::pep10::dot_uscall<address_size_t>::generate_listing_string() const {
    auto temp = fmt::format("{:<6} {:<6} {}", "", "", generate_source_string());

    return temp;
}

template <typename address_size_t> std::string asmb::pep10::dot_uscall<address_size_t>::generate_source_string() const {
    auto dot_string = ".USCALL";
    auto operand_string = argument->string();
    return fmt::format("{:<9}{:<8}{:<12}{}", "", dot_string, operand_string, this->get_formatted_comment());
}

template <typename address_size_t>
void asmb::pep10::dot_uscall<address_size_t>::append_object_code(std::vector<uint8_t> &bytes) const {
    return;
}